#include "../include/MemoryManager.hpp"
#include "../include/HybridPolicy.hpp"
#include <iostream>
#include <cassert>

void test_reference_list() {
    std::cout << "Testing Reference List..." << std::endl;

    auto policy = std::make_unique<HybridPolicy>(2); // RAM capacity of 2
    MemoryManager mm(10, 2, std::move(policy));

    // 1. Admit a process
    bool admitted = mm.admitProcess(1, 5);
    assert(admitted);
    std::cout << "Process 1 admitted with 5 pages." << std::endl;

    // 2. Add references
    mm.addReference(1, 0);
    mm.addReference(1, 1);
    mm.addReference(1, 0);
    mm.addReference(1, 2);

    const auto& list = mm.getReferenceList();
    assert(list.size() == 4);
    assert(mm.getNextReferenceIndex() == 0);
    std::cout << "4 references added to the list." << std::endl;

    // 3. Execute references one by one
    // Ref 0: PID 1, Page 0 (Page Fault)
    auto res1 = mm.executeNextReference();
    assert(res1.has_value() && res1.value() == 2);
    assert(mm.getNextReferenceIndex() == 1);
    std::cout << "Ref 1: Page Fault (Correct)" << std::endl;

    // Ref 1: PID 1, Page 1 (Page Fault)
    auto res2 = mm.executeNextReference();
    assert(res2.has_value() && res2.value() == 2);
    assert(mm.getNextReferenceIndex() == 2);
    std::cout << "Ref 2: Page Fault (Correct)" << std::endl;

    // Ref 2: PID 1, Page 0 (Hit)
    auto res3 = mm.executeNextReference();
    assert(res3.has_value() && res3.value() == 1);
    assert(mm.getNextReferenceIndex() == 3);
    std::cout << "Ref 3: Hit (Correct)" << std::endl;

    // 4. Run remaining
    // Ref 3: PID 1, Page 2 (Page Fault, triggers eviction)
    int executed = 0;
    while (mm.getNextReferenceIndex() < mm.getReferenceList().size()) {
        mm.executeNextReference();
        executed++;
    }
    assert(executed == 1);
    assert(mm.getNextReferenceIndex() == 4);
    std::cout << "Remaining references executed (Correct)" << std::endl;

    // 5. Clear
    mm.clearReferences();
    assert(mm.getReferenceList().empty());
    assert(mm.getNextReferenceIndex() == 0);
    std::cout << "List cleared (Correct)" << std::endl;

    std::cout << "All tests passed successfully!" << std::endl;
}

int main() {
    try {
        test_reference_list();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
