#include <defs.h>
#include <x86.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_clock.h>
#include <list.h>

// 用来保存clock替换算法中的保留在内存中的page
list_entry_t pra_list_head;
list_entry_t* current;

static int
_clock_init_mm(struct mm_struct *mm)
{     
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    return 0;
}

// 加入一个新的page到pra_list中，并将这个page对应的pte中的A位置位
static int
_clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    // 获取链表头
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    // 获取新加入的page的pra_page_link
    list_entry_t *entry=&(page->pra_page_link);
    // 更新current的位置
    current = entry;
    assert(entry != NULL && head != NULL);
    // 将新的pra_page_link加入到链表的队尾
    list_add_before(head, entry);
    // 获取page对应的pte 并标记pte中的PTE_A标志位
    pte_t *pte = get_pte(mm->pgdir,addr,0);
    *pte |= PTE_A;
    return 0;
}

// 根据clock算法选择一个需要替换出去的page
static int
_clock_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
    assert(in_tick==0);
    // 首先获取最近访问过的一个page对应的pra_list_entry
    struct Page* current_page = le2page(current, pra_page_link);
    cprintf("current page in vaddr 0x%08x \n", (current_page)->pra_vaddr);
    assert(current != NULL);
    struct Page * page = NULL;
    // 然后从这个entry开始遍历
    // 第一遍ckeck A==0 && D==0 这一遍不修改flag
    list_entry_t* index = current;
    while(index != current) {
        // 如果是pra_list_head直接continue
        if(index == &pra_list_head) {
            continue;
        }
        // 先获取当前链表项对应的page
        page = le2page(index, pra_page_link);
        // todo 这里有必要判断一下 page->ref != 0 吗
        // 获取page对应的pte
        pte_t* pte = get_pte(mm->pgdir, (uintptr_t)page->pra_vaddr, 0);
        // 如果A位和D位都无效 则说明这个page最近既没有访问过也没有被修改过
        if((*pte & (PTE_A | PTE_D)) == 0) {
            // 返回被置换出的page
            *ptr_page = page;
            // 调整 current
            current = list_next(index);
            // 将对应的pra_page_link从链表中删除
            list_del(index);
            cprintf("swap_out page in vaddr 0x%x PTE_A:0 PTE_D:0 \n", (*ptr_page)->pra_vaddr);
            return 0;
        }
        index = list_next(index);
    }
    // 第二遍check A==0 这一遍修改清空flag的A位
    index = current;
    while(index != current) {
        // 如果是pra_list_head直接continue
        if(index == &pra_list_head) {
            continue;
        }
        // 先获取当前链表项对应的page
        page = le2page(index, pra_page_link);
        // todo 这里有必要判断一下 page->ref != 0 吗
        // 获取page对应的pte
        pte_t* pte = get_pte(mm->pgdir, (uintptr_t)page->pra_vaddr, 0);
        // 如果A位无效 则说明这个page最近既没有访问过但被修改过
        if((*pte & (PTE_A)) == 0) {
            // 返回被置换出的page
            *ptr_page = page;
            // 调整 current
            current = list_next(index);
            // 将对应的pra_page_link从链表中删除
            list_del(index);
            cprintf("swap_out page in vaddr 0x%x PTE_A:0 PTE_D:1 \n", (*ptr_page)->pra_vaddr);
            return 0;
        }
        *pte &= ~PTE_A;
        index = list_next(index);
    }
    // 第三遍check D==0 这一遍时A位已经全部置0
    index = current;
    while(index != current) {
        // 如果是pra_list_head直接continue
        if(index == &pra_list_head) {
            continue;
        }
        // 先获取当前链表项对应的page
        page = le2page(index, pra_page_link);
        // todo 这里有必要判断一下 page->ref != 0 吗
        // 获取page对应的pte
        pte_t* pte = get_pte(mm->pgdir, (uintptr_t)page->pra_vaddr, 0);
        // 如果D位无效 则说明这个page最近虽然被访问过但是没有被修改过
        if((*pte & (PTE_D)) == 0) {
            // 返回被置换出的page
            *ptr_page = page;
            // 调整 current
            current = list_next(index);
            // 将对应的pra_page_link从链表中删除
            list_del(index);
            cprintf("swap_out page in vaddr 0x%x PTE_A:1 PTE_D:0\n", (*ptr_page)->pra_vaddr);
            return 0;
        }
        index = list_next(index);
    }
    // 到此为止 剩下的所有page都是最近被访问过又被修改过的
    // 随机挑选一个幸运观众换出即可
    // 但是考虑到 mm->sm_priv 中保存的是最近一次访问到的 所以优先不考虑这个
    if((index = list_next(current)) != &pra_list_head);
    else if((index = list_prev(current)) != &pra_list_head);
    else index = current;
    page = le2page(index, pra_page_link);
    // 返回被置换出的page
    *ptr_page = page;
    // 调整 current
    current = list_next(index);
    // 将对应的pra_page_link从链表中删除
    list_del(index);
    cprintf("swap_out page in vaddr 0x%08x PTE_A:1 PTE_D:1 \n", (*ptr_page)->pra_vaddr);
    return 0;
}

static int
_clock_check_swap(void) {
    cprintf("write Virt Page 0x00001000 in clock_check_swap\n");
    *(unsigned char*)0x1000 = 0x0a;
    assert(pgfault_num == 4);
    cprintf("write Virt Page 0x00002000 in clock_check_swap\n");
    *(unsigned char*)0x2000 = 0x0b;
    assert(pgfault_num == 4);
    cprintf("write Virt Page 0x00003000 in clock_check_swap\n");
    *(unsigned char*)0x3000 = 0x0c;
    assert(pgfault_num == 4);
    cprintf("write Virt Page 0x00004000 in clock_check_swap\n");
    *(unsigned char*)0x4000 = 0x0d;
    assert(pgfault_num == 4);
    /**
     *                                                                      current
     *  +--------------+     +--------------+     +--------------+     +--------------+
     *  | 0x1000 A1 D1 | --> | 0x2000 A1 D1 | --> | 0x3000 A1 D1 | --> | 0x4000 A1 D1 |
     *  +--------------+     +--------------+     +--------------+     +--------------+
     */
    // 将会发生缺页异常 而此时所有的 page PTE_A:1 PTE_D:1 
    // 替换出 0x3000
    cprintf("write Virt Page 0x00005000 in clock_check_swap\n");
    *(unsigned char*)0x5000 = 0x0e;
    assert(pgfault_num == 5);
    /**
     *                                                current
     *  +--------------+     +--------------+     +--------------+     +--------------+
     *  | 0x1000 A0 D1 | --> | 0x2000 A0 D1 | --> | 0x4000 A0 D1 | --> | 0x5000 A1 D1 |
     *  +--------------+     +--------------+     +--------------+     +--------------+
     */
    // 此时所有page的A位都被刷新为0 并且current指针停在0x4000
    // 此时再写0x4000会把A位置1
    cprintf("write Virt Page 0x00004000 in clock_check_swap\n");
    *(unsigned char*)0x4000 = 0x0b;
    assert(pgfault_num == 5);
    /**
     *                                                current
     *  +--------------+     +--------------+     +--------------+     +--------------+
     *  | 0x1000 A0 D1 | --> | 0x2000 A0 D1 | --> | 0x4000 A1 D1 | --> | 0x5000 A1 D1 |
     *  +--------------+     +--------------+     +--------------+     +--------------+
     */
    // 写0x3000 此时current指针在0x4000 并且0x4000和0x5000的A位和D位均为1
    // 应该会换出0x1000
    cprintf("write Virt Page 0x00003000 in clock_check_swap\n");
    *(unsigned char*)0x3000 = 0x0a;
    assert(pgfault_num == 6);
    /**
     *      current
     *  +--------------+     +--------------+     +--------------+     +--------------+
     *  | 0x2000 A0 D1 | --> | 0x4000 A0 D1 | --> | 0x5000 A1 D1 | --> | 0x3000 A1 D1 |
     *  +--------------+     +--------------+     +--------------+     +--------------+
     */
    // 访问0x2000 会使得A位为1
    assert(*(unsigned char*)0x2000 == 0x0b);
    /**
     *      current
     *  +--------------+     +--------------+     +--------------+     +--------------+
     *  | 0x2000 A1 D1 | --> | 0x4000 A0 D1 | --> | 0x5000 A1 D1 | --> | 0x3000 A1 D1 |
     *  +--------------+     +--------------+     +--------------+     +--------------+
     */
    // 此时写0x1000 会换出 0x4000
    *(unsigned char*)0x1000 = 0x0a;
    assert(pgfault_num == 7);
    /**
     *                            current
     *  +--------------+     +--------------+     +--------------+     +--------------+
     *  | 0x2000 A1 D1 | --> | 0x5000 A1 D1 | --> | 0x3000 A1 D1 | --> | 0x1000 A1 D1 |
     *  +--------------+     +--------------+     +--------------+     +--------------+
     */
    return 0;
}

static int
_clock_init(void)
{
    return 0;
}

static int
_clock_set_unswappable(struct mm_struct *mm, uintptr_t addr)
{
    return 0;
}

static int
_clock_tick_event(struct mm_struct *mm)
{
    return 0;
}


struct swap_manager swap_manager_clock =
{
     .name            = "clock swap manager",
     .init            = &_clock_init,
     .init_mm         = &_clock_init_mm,
     .tick_event      = &_clock_tick_event,
     .map_swappable   = &_clock_map_swappable,
     .set_unswappable = &_clock_set_unswappable,
     .swap_out_victim = &_clock_swap_out_victim,
     .check_swap      = &_clock_check_swap,
};