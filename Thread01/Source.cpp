#include "thread01.h"
#include "HeapSort.h"
#include "HeapSort2.h"
#include "HeapSortInlineComparison.h"
#include "AVLvsRBTreeSort.h"

void print_vector(std::vector<int>&& nums) {
    std::cout << "\nPrinting: ";
    for (int x : nums) {
        std::cout << x << ", ";
    }
    std::cout << "." << std::endl;
}

int factorial(int x) {
    int result = 1;
    if (x == 1) {
        return 1;
    }
    
    return factorial(x - 1) * x;
}

class Solutions {
public:
    int firstMissingPositive(std::vector<int>& nums) {
        int n = nums.size();

        for (int i = 0; i < n; i++) {
            if (
                nums[i] > 0 && nums[i] <= n &&
                nums[nums[i] - 1] != nums[i]
                ) {
                std::swap(nums[i], nums[nums[i] - 1]);
            }
        }

        print_vector(std::move(nums));

        for (int i = 0; i < n; i++) {
            if (nums[i] != i + 1)
                return i + 1;
        }

        return n + 1;
    }

};


void intialization() {
    int a1; 
    int a2 = 0; a1 = a2;//Copy
    int a3(5); //Direct

    std::string s1;
    std::string s2("C++");

    char d1[1]; // Uninitialised
    char d2[8] = { '\0' };

    char d3[8] = { '1', '2' };

    char d4[8] = { "abcd" };

    //Uniform initialization
    int b1{};
    //int b2(); //most vexing parse

    int b3{ 5 };
    int b4 = 0;

    char e1[8]{};
    char e2[8]{ "Hello" };
    
    int* p1 = new int{};

    char *p2 = new char[8] {};
    char *p3 = new char[8] {"Hello"};

    std::cout << "\na1: " << a1;
    std::cout << "\na2: " << a2;
    std::cout << "\na3: " << a3;

    std::cout << "\nd1: " << d1;
    std::cout << "\nd2: " << d2;
    std::cout << "\nd3: " << d3;
    std::cout << "\nd4: " << d4;


    std::cout << "\nb1: " << b1;
    std::cout << "\nb3: " << b3;
    std::cout << "\nb4: " << b4;


    std::cout << "\ne1: " << e1;
    std::cout << "\ne2: " << e2;
    
    std::cout << "\np1: " << p1;
    std::cout << "\np2: " << p2;
    std::cout << "\np3: " << p3;

}
void print_missing_number(std::vector<int>& nums) {
    Solutions solution;
    std::cout << "\nMissing number " << solution.firstMissingPositive(nums);
}

void swap(int* x, int* y) {
    int temp = *x;
    *x = *y;
    *y = temp;
}

void swapR(int& x, int& y) {
    int temp = x;
    x = y;
    y = temp;
}
void printPtr(int* ptr) {
    std::cout << "*ptr: " << *ptr << std::endl;
}
void swap_test() {
    int x = 5;
    int y = 20;

    std::cout << "Before x: " << x << ", y: " << y << std::endl;
    swap(&x, &y);
    std::cout << "After  x: " << x << ", y: " << y << std::endl;

    x = 200;
    y = -500;

    std::cout << "Before x: " << x << ", y: " << y << std::endl;
    swapR(x, y);
    std::cout << "After  x: " << x << ", y: " << y << std::endl;

}

int main() {
    swap_test();
    if (true) {
        return 0;
    }
    avl_vs_rb_tree_sort_main();
    heap_sort_inline_comparison_main();
    heap_sort_main();
    heap_sort_2_main();

    intialization();

    int x = 2;
    std::cout << "\nfactorial: " << x << " " << factorial(x);
    
    x = 3;
    std::cout << "\nfactorial: " << x << " " << factorial(x);

    x = 4;
    std::cout << "\nfactorial: " << x << " " << factorial(x);

    x = 8;
    std::cout << "\nfactorial: " << x << " " << factorial(x);

    std::cout << "From Source, via thread.h "  << std::endl;
	//thread_main();
    std::vector<int> nums = { 1,2,3 };
    print_missing_number(nums);

    nums = { -1,2,3 };
    print_missing_number(nums);

    nums = { -1, 1, 2, 3, 5 };
    print_missing_number(nums);

    nums = { -1,1,2, 3, 3 , 3, 5 };
    print_missing_number(nums);

    nums = { -1,1,2,3,4,6 };
    print_missing_number(nums);

    nums = { -1,1,2,3,6 };
    print_missing_number(nums);
}