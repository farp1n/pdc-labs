#include <iostream>
#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <string>
#include <chrono>
#include <Windows.h> // Для Sleep() або використання std::this_thread::sleep_for

using namespace std;

// ==========================================
// ГЛОБАЛЬНІ ЗМІННІ ДЛЯ ПОПЕРЕДНІХ ПУНКТІВ
// ==========================================
list<int> global_list;
mutex mtx_global;


// ==========================================
// 1.2.1 та 1.2.2: Прості потоки та detach
// ==========================================
void Thread1_Func() {
    cout << "Thread 1 output: 1\n";
}

void Thread2_Func() {
    cout << "Thread 2 output: 2\n";
}

void RunDemo_1_2_1_and_1_2_2() {
    cout << "\n=== DEMO 1.2.1 & 1.2.2 (Threads & Detach) ===\n";
    thread t1(Thread1_Func);
    thread t2(Thread2_Func);

    // Використовуємо detach, як вимагається у пункті 1.2.2
    t1.detach();
    t2.detach();

    this_thread::sleep_for(chrono::milliseconds(50)); // Чекаємо виводу
}


// ==========================================
// 1.2.3 та 1.2.4: Список із м'ютексом і без
// ==========================================
void AddToList_Unsafe(int start_val) {
    global_list.push_back(start_val);
    cout << "[Unsafe] Added: " << start_val << endl;
    for (int i = 1; i <= 9; ++i) {
        global_list.push_back(start_val + i);
        cout << "[Unsafe] Added: " << (start_val + i) << endl;
        this_thread::sleep_for(chrono::milliseconds(5));
    }
}

void ListContains_Unsafe(int target_val) {
    for (int i = 0; i < 10; ++i) {
        bool found = false;
        for (int val : global_list) {
            if (val == target_val) { found = true; break; }
        }
        cout << "[Unsafe Check] Value " << target_val << (found ? " IN list\n" : " NOT in list\n");
        this_thread::sleep_for(chrono::milliseconds(5));
    }
}

// Захищена м'ютексом версія (1.2.4)
void AddToList_Safe(int start_val) {
    mtx_global.lock();
    global_list.push_back(start_val);
    cout << "[Safe] Added: " << start_val << endl;
    mtx_global.unlock();

    for (int i = 1; i <= 9; ++i) {
        mtx_global.lock();
        int val = start_val + i;
        global_list.push_back(val);
        cout << "[Safe] Added: " << val << endl;
        mtx_global.unlock();
        this_thread::sleep_for(chrono::milliseconds(5));
    }
}

void ListContains_Safe(int target_val) {
    for (int i = 0; i < 10; ++i) {
        mtx_global.lock();
        bool found = false;
        for (int val : global_list) {
            if (val == target_val) { found = true; break; }
        }
        cout << "[Safe Check] Value " << target_val << (found ? " IN list\n" : " NOT in list\n");
        mtx_global.unlock();
        this_thread::sleep_for(chrono::milliseconds(5));
    }
}

void RunDemo_1_2_3_to_1_2_5() {
    cout << "\n=== DEMO 1.2.3 - 1.2.5 (List & Mutex / Lock_Guard) ===\n";
    global_list.clear();

    // Демонстрація безпечного варіанту з lock_guard (пункт 1.2.5)
    auto Add_LockGuard = [](int val) {
        lock_guard<mutex> lock(mtx_global);
        global_list.push_back(val);
        cout << "[LockGuard] Added: " << val << endl;
        };

    auto Check_LockGuard = [](int target) {
        lock_guard<mutex> lock(mtx_global);
        bool found = false;
        for (int v : global_list) if (v == target) found = true;
        cout << "[LockGuard Check] Target " << target << (found ? " found\n" : " not found\n");
        };

    vector<thread> threads;
    int base_val = 50;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(Add_LockGuard, base_val++);
        threads.emplace_back(Check_LockGuard, 52);
    }

    for (auto& t : threads) t.detach();
    this_thread::sleep_for(chrono::milliseconds(100));
}


// ==========================================
// 1.2.6 та 1.2.7: Клас someData, exchangePerson
// ==========================================
struct someData {
    string name;
    string surname;
    string address;
    int age;

    void print(const string& prefix) const {
        cout << prefix << " -> Name: " << name << ", Surname: " << surname
            << ", Address: " << address << ", Age: " << age << endl;
    }
};

class exchangePerson {
private:
    someData data;
    mutable mutex mtx;

public:
    exchangePerson(string n, string s, string a, int ag) : data{ n, s, a, ag } {}

    static void JohnDoe(exchangePerson& person) {
        lock_guard<mutex> lock(person.mtx);
        person.data.name = "John";
        person.data.surname = "Doe";
        person.data.address = "Unknown";
        person.data.age = 120;
    }

    static void JacobSmith(exchangePerson& person) {
        lock_guard<mutex> lock(person.mtx);
        person.data.name = "Jacob";
        person.data.surname = "Smith";
        person.data.address = "Known";
        person.data.age = 1;
    }

    // Метод Swap із застосуванням unique_lock та defer_lock (згідно з пунктом 1.2.7)
    static void Swap(exchangePerson& p1, exchangePerson& p2) {
        if (&p1 == &p2) {
            cout << "Error: Cannot swap object with itself!\n";
            return;
        }

        // Використовуємо unique_lock з defer_lock (вимога п. 1.2.7)
        unique_lock<mutex> lock1(p1.mtx, defer_lock);
        unique_lock<mutex> lock2(p2.mtx, defer_lock);

        // Безпечне блокування обох м'ютексів без взаємного блокування (deadlock)
        std::lock(lock1, lock2);

        // Обмін даними
        someData temp = p1.data;
        p1.data = p2.data;
        p2.data = temp;

        cout << "[Swap] Swap operation completed successfully between two objects.\n";
    }

    void printData(const string& msg) const {
        lock_guard<mutex> lock(mtx);
        data.print(msg);
    }
};

void RunDemo_1_2_6_and_1_2_7() {
    cout << "\n=== DEMO 1.2.6 & 1.2.7 (exchangePerson & Swap) ===\n";
    exchangePerson person1("Alice", "Brown", "Kyiv", 25);
    exchangePerson person2("Bob", "White", "Lviv", 30);

    cout << "[Initial State]:\n";
    person1.printData("Person 1");
    person2.printData("Person 2");

    // Запуск JohnDoe та JacobSmith в окремих потоках з від'єднанням (detach)
    thread t1(exchangePerson::JohnDoe, ref(person1));
    thread t2(exchangePerson::JacobSmith, ref(person2));
    t1.detach();
    t2.detach();

    this_thread::sleep_for(chrono::milliseconds(50));

    cout << "\n[After JohnDoe & JacobSmith threads]:\n";
    person1.printData("Person 1");
    person2.printData("Person 2");

    // Запуск Swap в окремому потоці з очікуванням завершення (join)
    thread tSwap(exchangePerson::Swap, ref(person1), ref(person2));
    tSwap.join(); // Гарантуємо отримання результатів до завершення головного потоку

    cout << "\n[After Swap operation]:\n";
    person1.printData("Person 1");
    person2.printData("Person 2");
}


// ==========================================
// ГОЛОВНА ФУНКЦІЯ MAIN
// ==========================================
int main() {
    // Налаштування української локалі для коректного виводу в консоль
    system("chcp 65001 > nul");

    cout << "LABORATORY WORK #1 STARTING...\n";

    // Послідовний виклик усіх демонстрацій лабораторної роботи
    RunDemo_1_2_1_and_1_2_2();
    RunDemo_1_2_3_to_1_2_5();
    RunDemo_1_2_6_and_1_2_7();

    cout << "\nLABORATORY WORK FINISHED SUCCESSFULLY.\n";
    return 0;
}