# A note about KVM first
I often hear people say "we're running this on KVM". That makes me cringe because you don't run a VM on KVM, you run it in QEMU.
KVM is simply a linux module that gives you access to the CPU Virtualization features. QEMU emulates the machine (netcard, gfx, keyboard, mouse, chipset, disk, bios etc..). KVM is only the part that emulates the CPU. And in fact, it doesn't emulate it ...

Intel (and AMD) has a feature that allows you to set the CPU in "virtualization mode". I've talked about this in the past: http://www.dumais.io/index.php?article=ac3267239dd3e34c061de6413203fb98
So what KVM does is kind of like creating virtual CPUs but when your code runs on those vCPUs, it is running on the REAL CPU. It's just running inside a different thread (or rather some kind of thread on steroids)

# A virtual machine emulator
This project is only a proof of concept. But it shows how it would be possible to emulate a new kind of x86 based machine. It would be a machine specifically built to make it easy to write an OS. It still
uses a x86 CPU but it doesn't have a BIOS and a PCI bus, and it doesn't have any of the legacy things that are still present in a PC today for compatibility reasons (like a PIC, or VGA memory region).

It shows how a x86 CPU could be used in a machine that is totally different than the well known PC.
    - It provives a x86 CPU and memory using 
        - so it can deliver interrupts (IRQ)
    - Provices virtual hardware that is very easy to interface 
        - Network card with a TAP interface as the backend
        - serial port
        - graphics card as a VNC server
        - keyboard (from the VNC server)
    - It currently doesn't implement a timer. Which is crucial
    - It doesn't support multi-CPU yet but I plan on implementing 
      it a way that it would be even easier to use for the guest

The devices are also not emulated the same way it works on a PC. For example, I wrote my own, very naive, implementation of what would be a PCI bus. But it achieves the same goal: The HV creates a block of information that contains details about devices connected on the virtual machine within the guest's memory. So the guest can scan that block of info to discover the available devices. Normally, the BIOS would build that info. But within a hypervisor, you can let the HV create this in the guest memory before starting it up.

The BIOS would normally load the first sector of the main HDD in memory. Again, I don't need that because I can instruct the HV to directly load the full kernel at memory location 0x00000000 and make the VM start from that address (instead of 0x7C00). So no need for a booloader either.

This makes this project a very convenient hypervisor to develop a hobby OS. We can focus on writting an OS instead of all the auxiliary stuff. For example:
Focus on:

    - Switching to long mode
    - creating the page tables and the memory manager
    - slightly easier discovery of devices than using the PCI standard
    - creating a scheduler
    - creating a block layer and FS layer
    - creating a terminal and a GUI

What you don't have to do:

    - A bootloader
    - writing a PCI driver(kindof ...)
    - writing complicated device drivers. The devices implemented 
      are even easier to interface than virtio
    - Dealing with the A20 line
    - Dealing with reserved memory for BIOS


# Support multiple CPUs
The nice thing about KVM is that it also handles the CPU bootstraping in a SMP setup.
So when creating a multi-CPU VM, you can simply run KVM_RUN on all vcpu and KVM will
automatically stall the AP vcpus until the BST sends the INIT IPI.

The hypervisor creates 1 thread per vcpu and calls KVM_RUN (VMRESUME) in a loop.


# Guest machine
The guest code is very messy. That's because I didn't wanna bother rewriting another OS, I've those this already in the past (https://github.com/pdumais/OperatingSystem)


# Final Thought
I've written an OS in the past, as mentioned above. It was fun to learn about how PCI works, how to write virtio drivers, how to deal with the BIOS, writting a bootloader, etc. But I think it would've been nice if all of that had been simplified since this is not where I wanted to spend my limited time on. So I think that a "simple machine emulator" has its place in the world. 

# License
I really don't know. I think I have to GPL my code because it uses libvncserver. But I don't care if anyone uses the code I wrote for any purposes. I would use Apache, but I think I have to spread GPL.
Whatever ....