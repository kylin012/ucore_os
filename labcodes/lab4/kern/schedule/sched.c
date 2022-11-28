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
//只能由idleproc完成調度，其他進程不得調用
//在cpu_idle中循環調用
void
schedule(void) {
    bool intr_flag;
    list_entry_t *le, *last;
    struct proc_struct *next = NULL;
    local_intr_save(intr_flag);//関中斷，上下文切換過程不能響應任何外部/内部中斷
    {
        current->need_resched = 0;
        //idleproc并沒有鏈接在proc_list中，用表頭表示
        last = (current == idleproc) ? &proc_list : &(current->list_link);
        le = last;
        do {//遍歷proc_list，直到找到下一個是runnable的節點
            if ((le = list_next(le)) != &proc_list) {//跳過表頭
                next = le2proc(le, list_link);
                if (next->state == PROC_RUNNABLE) {
                    break;
                }
            }
        } while (le != last);
        if (next == NULL || next->state != PROC_RUNNABLE) {//proc_list爲空或沒有就緒進程，調度idleproc
            next = idleproc;
        }
        next->runs ++;//進程調度次數纍加
        if (next != current) {//如果成功切換，開始運行新進程
            proc_run(next);//need_resched在這裏被置爲1
        }
    }
    local_intr_restore(intr_flag);//打開中斷
}

