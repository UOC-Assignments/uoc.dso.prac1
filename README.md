# uoc.dso.prac1
Operative Systems Design - Practical Assignment #1: Implement a Linux kernel module that counts every exit system call executed and also allows access to a summary from user space interfacing via proc fs (/proc/traceexit) 

/*###############################################################################
 #                                                                              #
 #                       UOC - Open University of Catalonia                     #
 #                                                                              #
 ################################################################################
 ################################################################################
 #                                                                              #
 #                         OPERATIVE SYSTEMS DESIGN (DSO)                       #
 #                             PRACTICAL ASSIGNMENT 1                           #
 #                                                                              #
 #                        STUDENT: Jordi Bericat Ruz                            #
 #                           TERM: Autumn 2020/21                               #
 #                       GIT REPO: UOC-Assignments/uoc.dso.prac1"               #
 #                                                                              #
 #                            FILE 1 OF 1: traceexit.c                          #
 #                                                                              #
 #                                  Version 1.1                                 #
 #                                                                              #
 ################################################################################
 ################################################################################
 #                                                                              #
 #  DESCRIPTION:                                                                #
 #                                                                              #
 #  Implement a Linux Kernel module that keeps track of every exit system call  #
 #  executed and also allows access to a summary from user space (interfacing   #
 #  via procfs).                                                                #
 #                                                                              #
 #                                                                              #
 #  IMPLEMENTATION STRATEGY:                                                    #
 #                                                                              #
 #  Exit System calls monitoring                                                #
 #                                                                              #
 #  In regards to the exit syscall codes monitoring, we only have to call the   #
 #  low-level assembler code which performs the sys_exit call associated tasks  #
 #  from within a new function (new_sys_exit) and then link it to the system    #
 #  calls vector (sys_call_table). Summing-up: this way we bypass the original  #
 #  sys_exit call (original_sys_exit) so we can modify its behaviour (in our    #
 #  case, to keep track of every exit system call executed once the kernel      #
 #  module becomes enabled via insmod traceexit.ko).                            #
 #                                                                              #
 #  Procfs Interface implementation                                             #
 #                                                                              #
 #  The goal is to implement a communication interface between the user memory  #
 #  space and the one associated with the kernel module, so an user will be     #
 #  able to read from the shell (user memory space) the data stored in the      #
 #  physical memory space reserved to the kernel (exit syscalls counters) via   #
 #  the procfs "virtual" interface -> cat /proc/traceexit. Writing operations   #
 #  will be performed from within the module (kernel memory space), so a        #
 #  writing interface from user to kernel space won't be needed.                #
 #                                                                              #
 #                                                                              #
 #  INPUT:                                                                      #
 #                                                                              #
 #  N/A                                                                         #
 #                                                                              #
 #                                                                              #
 #  OUTPUT:                                                                     #
 #                                                                              #
 #  Summary at /proc/traceexit showing the amount of times every exit code      #
 #  that has been invocated since the kernel module activation.                 #
 #                                                                              #
 #                                                                              #
 #  USAGE:                                                                      #
 #                                                                              #
 #  See examples/usage.txt                                                      #
 #                                                                              #
 ################################################################################
