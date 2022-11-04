#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>

#define LEFT_LEAF(index) ((index)*2)
#define RIGHT_LEAF(index) ((index)*2 + 1)
#define PARENT(index) (((index) + 1) / 2 - 1)

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

static struct Page*
buddy_alloc_pages(size_t n) {

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