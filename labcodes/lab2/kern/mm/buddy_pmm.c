#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>

#define LEFT_LEAF(index) ((index)*2)
#define RIGHT_LEAF(index) ((index)*2 + 1)
#define PARENT(index) (index / 2)

#define IS_POWER_OF_2(x) (!((x) & ((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int ROUND_UP_LOG(int size){
    int n = 0, tmp = size;
    while(tmp >>= 1){
        n++;
    }
    tmp = (size >> n) << n;
    n += ((size - tmp) !=0);
    return n;
}

unsigned* buddy_manager;

struct Page* page_base;

int free_pages, manager_size;

static void
buddy_init(void) {
    
}

static void
buddy_init_memmap(struct Page *base, size_t n){
    // 首先对页进行初始化
    struct Page* p;
    for(p = base; p != base + n; p++){
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
        SetPageProperty(p);
    }
    // 获取 buddy_manager 数组的长度
    manager_size = 2 * (1<<ROUND_UP_LOG(n));
    // 获取 buddy_manager 的起始地址 预留base最开始的部分给 buddy_manager
    buddy_manager = (unsigned*) page2kva(base);
    // 调整 base
    base += 4 * manager_size / 4096;
    // page_base 用来在后续申请和释放内存的时候提供可用内存空间的起始地址
    page_base = base;
    // 剩余可用页数
    free_pages = n - 4 * manager_size / 4096;
    // buddy数组的下标[1 … manager_size]有效
    int i = 1;
    int node_size = manager_size / 2;
    for(; i < manager_size; i++){
        buddy_manager[i] = node_size;
        if(IS_POWER_OF_2(i+1)){
            node_size /= 2;
        }
    }
    base->property = free_pages;
    SetPageProperty(base);
    cprintf("===================buddy init end===================\n");
    cprintf("free_size = %d\n", free_pages);
    cprintf("buddy_size = %d\n", manager_size);
    cprintf("buddy_addr = 0x%08x\n", buddy_manager);
    cprintf("manager_page_base = 0x%08x\n", page_base);
    cprintf("====================================================\n");
}

int 
buddy_alloc(int size){
    unsigned index = 1;
    unsigned offset = 0;
    unsigned node_size;
    // 将size向上取整为2的幂
    if(size <= 0)
        size = 1;
    else if(!IS_POWER_OF_2(size))
        size = 1 << ROUND_UP_LOG(size);
    if(buddy_manager[index] < size)
        return -1;
    // 从根节点往下深度遍历，找到恰好等于size的块
    for(node_size = buddy_manager[index];node_size != size;node_size /= 2){
        if(buddy_manager[LEFT_LEAF(index)] >= size)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);
    }
    // 将找到的块取出分配
    buddy[index] = 0;
    // 计算块在所有块内存中的索引
    offset = (index + 1) * node_size - manager_size / 2;
    // 向上回溯至根节点，修改沿途节点的大小
    while(index > 1){
        index = PARENT(index);
        buddy_manager[index] = MAX(buddy_manager[LEFT_LEAF(index)],buddy_manager[RIGHT_LEAF(index)]);
    }
    return offset;
}

static struct Page*
buddy_alloc_pages(size_t n) {
    assert(n>0);
    if(n > free_pages)
        return NULL;
    // 获取分配的页在内存中的起始地址
    int offset = buddy_alloc(n);
    struct Page *base = page_base + offset;
    struct Page *page;
    // 将n向上取整，因为有round_n个块被取出
    round_n = 1 << ROUND_UP_LOG(n);
    // 将每一个取出的块由空闲态改为保留态
    for(page = base;page != base + round_n;page++){
        ClearPageProperty(page);
        SetPageReserved(page);
    }
    free_pages -= round_n;
    base->property = n;
    return base;
}

static void
buddy_free_pages(struct Page* base, size_t n) {

}

static size_t
buddy_nr_free_pages(void) {

}

static void
basic_check(void) {

}

static void
buddy_check(void) {

}

const struct pmm_manager default_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};