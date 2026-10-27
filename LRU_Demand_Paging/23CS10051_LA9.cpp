#include <iostream>
#include <array>
#include <iomanip>
#include <cstdlib>
using namespace std;

static constexpr int PAGE_FAULT_TIME = 10000;
static constexpr int MEM_ACC_TIME = 100;
static constexpr int PAGE_SZ = 4 * 1024;
static constexpr int PAGE_TABLE_SZ = 5000;

long long memAccess = 0;
long long pageFault = 0;

void clear()
{
    memAccess = 0;
    pageFault = 0;
}

double calculate_time()
{
    return pageFault * (double)PAGE_FAULT_TIME * 1e-6 + memAccess * (double)MEM_ACC_TIME * 1e-9;
}

void printStats()
{
    cout << "Accesses: " << memAccess;
    cout << "  Faults: " << pageFault;
    cout << "  Fault rate: " << fixed << setprecision(2) << 100.0 * pageFault / memAccess << "%";
    cout << "  Estimated time: " << fixed << setprecision(3) << calculate_time() << " s\n";
}

typedef struct Page
{
    Page *prev;
    Page *next;
    int val;
    Page(int Val, Page *prevptr, Page *nextptr)
        : prev(prevptr), next(nextptr), val(Val) {}
} Page;

class LRU
{
    Page *head;
    Page *last;
    static array<Page *, PAGE_TABLE_SZ> PT;
    int F;
    int currSize;

public:
    LRU(int f)
        : head(nullptr), last(nullptr), F(f), currSize(0)
    {
        for (auto &page : PT)
            page = nullptr;
    }

    void reset()
    {
        while (head != last)
        {
            Page *temp = head;
            head = head->next;
            delete temp;
        }
        if (head != nullptr)
        {
            delete head;
            head = nullptr;
        }
        last = nullptr;
        currSize = 0;
        PT.fill(nullptr);
    }

    void access_page(int page)
    {
        if (PT[page] == nullptr)
        {
            ++pageFault;
            insert(page);
        }
        else
        {
            update_LRU(PT[page]);
        }
    }

    ~LRU()
    {
        while (this->head != this->last)
        {
            Page *temp = this->head;
            this->head = this->head->next;
            delete temp;
        }
        if (this->head != nullptr)
            delete this->head;
    }

private:
    void update_LRU(Page *page)
    {
        if (page == head)
            return;
        if (page->prev != nullptr)
            page->prev->next = page->next;
        if (page->next != nullptr)
            page->next->prev = page->prev;
        else
            last = page->prev;
        page->next = head;
        page->prev = nullptr;
        if (head)
            head->prev = page;
        head = page;
    }

    void insert(int val)
    {
        if (val < 0 || val >= PAGE_TABLE_SZ)
        {
            cerr << "(ERROR) LRU:insert: val not bound: " << val << "\n";
            return;
        }
        if (currSize < F)
        {
            Page *p = new Page(val, nullptr, head);
            if (head)
                head->prev = p;
            head = p;
            if (last == nullptr)
                last = p;
            ++currSize;
            PT[val] = head;
        }
        else
        {
            PT[last->val] = nullptr;
            PT[val] = last;
            last->val = val;
            update_LRU(last);
        }
    }
};

array<Page *, PAGE_TABLE_SZ> LRU::PT;

// Each benchmark makes 20000 virtual memory accesses.
// page = address / PAGE_SZ.
void run_test(LRU &lru, int type)
{
    srand(42);

    for (int i = 0; i < 20000; i++)
    {
        int page = 0;

        if (type == 0) // sequential addresses, 32 accesses per page
            page = (i / 32) % 256;
        else if (type == 1) // random pages
            page = rand() % 256;
        else // 80% of accesses to 16 frequently used pages
        {
            if (rand() % 100 < 80)
                page = rand() % 16;
            else
                page = rand() % 256;
        }

        long long address = (long long)page * PAGE_SZ + (i % 32);
        memAccess++;
        lru.access_page((int)(address / PAGE_SZ));
    }
}

int main()
{
    int frames[] = {4, 8, 16, 32};
    const char *names[] = {"Sequential", "Random", "Locality-heavy"};

    for (int type = 0; type < 3; type++)
    {
        cout << "\n" << names[type] << " access pattern\n";

        for (int f : frames)
        {
            LRU lru(f);
            clear();
            run_test(lru, type);

            cout << "Frames: " << f << "  ";
            printStats();
        }
    }
    return 0;
}
