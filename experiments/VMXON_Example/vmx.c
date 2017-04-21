#include <linux/init.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <asm/errno.h>
#include <linux/slab.h>
#include <asm/msr.h>
#include <asm/bitops.h>
#include <asm/page_types.h>

MODULE_LICENSE("Dual BSD/GPL");

/*MACROS*/
#define ASM_VMX_VMXON_RAX  ".byte 0xf3, 0x0f, 0xc7, 0x30"
#define ASM_VMX_VMXOFF     ".byte 0x0f, 0x01, 0xc4"

#define CR4_VMXE           0x2000

/*DEFINITIONS */

static struct vmcs_descriptor{
   int size;
   int order;
   u32 revision_id;
} vmcs_descriptor;

struct vmcs{
   u32 revision_id;
   u32 abort_data;
   char data[0];
};

/* GLOBAL VARS */
struct vmcs *vmcs;

static __init void setup_vmcs_descriptor(void){
   u32 vmx_msr_low,vmx_msr_high;

   rdmsr(MSR_IA32_VMX_BASIC, vmx_msr_low,vmx_msr_high);
   vmcs_descriptor.size = vmx_msr_high & 0x1fff;
   vmcs_descriptor.order = get_order(vmcs_descriptor.size);
   vmcs_descriptor.revision_id = vmx_msr_low;
}

static __init int vmx_available(void)
{
   unsigned long ecx = cpuid_ecx(1);
   return test_bit(5, &ecx);
}

static __init int vmx_disabled_by_bios(void)
{
   u64 msr;

   rdmsrl(MSR_IA32_FEATURE_CONTROL, msr);
   return (msr & 5) != 5;
}

static __init int allocate_vmcs(void)
{
   struct page *pages;

   //Allocate page for the VMXON region
   pages = alloc_pages(GFP_KERNEL, vmcs_descriptor.order);

   if( !pages ){
      printk(KERN_ALERT "Error allocating memory for the vmcs\n");
      return -ENOMEM;
   }

   vmcs = page_address( pages );
   memset(vmcs, 0, vmcs_descriptor.size);
   vmcs->revision_id = vmcs_descriptor.revision_id;
   return 0;
}

static  int check_cflag(void)
{
   u16 flags;

   __asm__ __volatile__(
         "LAHF;"
         : "=a" (flags)
   );

   return ( flags & 0x1<<8 );
}

static int check_zflag(void)
{
   u16 flags;

   __asm__ __volatile__(
         "LAHF;"
         : "=a" (flags)
   );


   return ( flags & 0x40<<8 );
}

static inline unsigned long read_cr4(void)
{
    unsigned long val;
    asm volatile("mov %%cr4,%0\n\t" : "=r" (val), "=m" (__force_order));
    return val;
}

static inline void write_cr4(unsigned long val)
{
    asm volatile("mov %0,%%cr4": : "r" (val), "m" (__force_order));
}

static __init int hw_enable(void)
{
    u64 phys_addr = __pa(vmcs);

    write_cr4(read_cr4() | CR4_VMXE);
    __asm__ __volatile__ (ASM_VMX_VMXON_RAX : : "a"(&phys_addr), "m"(phys_addr)
	    : "memory", "cc");

    //return 1 if flags.cf is unset
    return (!check_cflag());
}

static __exit int hw_disable(void)
{
    //disable VMX instructions
    __asm__ __volatile__ (ASM_VMX_VMXOFF : : : "cc");

    //check if everything's good
    //return 1 if flags.cf & flags.zf are unset
    return ( !( check_cflag() && check_zflag() ) );

}

static __init int vmx_init(void)
{

    if ( !vmx_available() ){
	printk(KERN_ALERT "VMX: VMX instructions not available\n");
	return -EOPNOTSUPP;
    }

    if ( vmx_disabled_by_bios() ){
	printk(KERN_ALERT "VMX: VMX instructions disabled by bios\n");
	return -EOPNOTSUPP;
    }

    //setup a vmcs descriptor/VMXON region
    setup_vmcs_descriptor();

    //allocate a vmcs region in memory requesting a 4KB page
    allocate_vmcs();

    // if cflag is set the VMXON instruction wasn't correctly executed
    if( hw_enable() )
	printk( KERN_INFO "VMX: VMXON executed correctly\n");
    else
	printk( KERN_ALERT "VMX: VMXON not correctly executed\n");

    return 0;
}

static __exit void vmx_exit(void)
{
    free_pages( (unsigned long) vmcs, vmcs_descriptor.order);

    if ( hw_disable() )
	printk( KERN_INFO "VMXOFF executed correctly\n");
    else
	printk( KERN_ALERT "VMXOFF not correctly executed\n");
}

module_init(vmx_init);
module_exit(vmx_exit);
