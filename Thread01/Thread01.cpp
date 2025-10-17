// coutUnsynchronized.cpp
#include "thread01.h"

class Worker {
public:
    Worker(std::string n) :name(n) {
        std::cout << "Work created for " << n << std::endl;
    };

    Worker& operator=(const Worker&) = delete;
    
    Worker(const Worker& worker) = delete;
    /*{
        name = worker.name;
        work = std::vector<std::string>(worker.work);
        std::cout << "Creating a copy of: " << name <<std::endl;
    }*/

    void operator() () const {

    }
    void operator() () {
        
        for (int i = 1; i <= 3000; ++i) {
            // begin work
            //std::this_thread::sleep_for(std::chrono::milliseconds(2));      // (3)
            // end work
            std::stringstream ss;

            ss << name << ": " << "Work " << i << " done !!!" << '\n';
            work.push_back(ss.str()); // (4)
            ss.flush();
        }
        std::cout << getLastWork() << std::endl;
    }
    std::string getLastWork() {
        if (work.size() == 0) {
            std::cout << "name: " << name << ", work.size() " << work.size() << std::endl;
            0;
        }
        else {
            std::cout << "name: " << name << ", WORK.SIZE() " << work.size() << std::endl;
            return work[work.size() - 1];
        }
    }

    ~Worker() {
        std::cout << "destructor called: " << name << ", work.size() " << work.size() << std::endl;
    }
private:
    std::string name;
    std::vector<std::string> work;
};

void exposeMoreVariable(const Worker& workerRef) {
    workerRef();
}

int thread_main() {

    std::cout << '\n';

    std::cout << "Boss: Let's start working.\n\n";
    Worker herbW("herb");
    Worker andreiW("  Andrei");
    Worker scottW("    Scott");
    Worker bjarneW("      Bjarne");
    Worker bartW("        Bart");
    Worker jenneW("          Jenne");
    std::thread herb = std::thread(std::ref(herbW));                        // (1)
    std::thread andrei = std::thread(std::ref(andreiW));
    std::thread scott = std::thread(std::ref(scottW));
    std::thread bjarne = std::thread(std::ref(bjarneW));
    std::thread bart = std::thread(std::ref(bartW));
    std::thread jenne = std::thread(std::ref(jenneW));            // (2)

    
    herb.join();
    andrei.join();
    scott.join();
    bjarne.join();
    bart.join();
    jenne.join();

    //std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    std::cout << "\n from Main, " << herbW.getLastWork() << '\n';                   // (4.1)
    std::cout << "\n from Main, " << andreiW.getLastWork() << '\n';                   // (4.1)
    std::cout << "\n from Main, " << scottW.getLastWork() << '\n';                   // (4.1)
    std::cout << "\n from Main, " << bjarneW.getLastWork() << '\n';                   // (4.1)
    std::cout << "\n from Main, " << bartW.getLastWork() << '\n';                   // (4.1)
    std::cout << "\n from Main, " << jenneW.getLastWork() << '\n';                   // (4.1)

    std::cout << "\n" << "Boss: Let's go home." << '\n';                   // (5)


    std::cout << '\n';
    return 0;

}

// use a std::ref(obj) way of creating the thread object,
// which additionally is supported via making the assignment operator and copy constructor to be skipped from deafult added by a compiler.


// Adding some destructor code to have the visibility of RAII way of destruction getting called when the object is no longer in scope. 