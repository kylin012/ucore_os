.text
.globl kernel_thread_entry
# 为内核线程的主体函数fn做一个准备开始和结束运行的“外壳”
kernel_thread_entry:        # void kernel_thread(void)

    pushl %edx              # push arg
    call *%ebx              # call fn

    pushl %eax              # save the return value of fn(arg)
    call do_exit            # call do_exit to terminate current thread

