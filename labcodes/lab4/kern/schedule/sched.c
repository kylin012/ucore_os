#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <assert.h>

void
wakeup_proc(struct proc_struct *proc) {
    assert(proc->state != PROC_ZOMBIE && proc->state != PROC_RUNNABLE);
    proc->state = PROC_RUNNABLE;
}

// 只能由 idleproc 完成调度，其他进程不能主动进行调度
// 在cpu_idle中循环调用
void
schedule(void) {
    bool intr_flag;
    list_entry_t *le, *last;
    struct proc_struct *next = NULL;
    // 关中断，上下文切换过程不能响应任何外部/内部中断
    local_intr_save(intr_flag);
    {
        current->need_resched = 0;
        // idleproc 并沒有链接在 proc_list 中，用表头表示
        last = (current == idleproc) ? &proc_list : &(current->list_link);
        le = last;
        // 遍历 proc_list 直到找到下一个是 runnable 的节点
        do {
            // 跳过表头
            if ((le = list_next(le)) != &proc_list) {
                next = le2proc(le, list_link);
                if (next->state == PROC_RUNNABLE) {
                    break;
                }
            }
        } while (le != last);
        // 当proc_list为空没有就绪进程，调度 idleproc
        if (next == NULL || next->state != PROC_RUNNABLE) {
            next = idleproc;
        }
        // 被调度的进程的调度次数递增
        next->runs ++;
        // 只有发生了真正的进程调度，才需要执行proc_run()函数
        if (next != current) {
            // need_resched在这里被重置为1
            proc_run(next);
        }
    }
    // 打开中断
    local_intr_restore(intr_flag);
}

