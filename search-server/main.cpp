#include <iostream>
#include <string>
/**
 *  Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
 *  Напишите ответ здесь:
 *  271 комбинация, код ниже
 */
using namespace std;

int main() {
    int result = 0;
    for(int i = 1; i < 1000; ++i) {
        if(to_string(i).find('3') == std::string::npos) continue;
        ++result;
        cout << "Combination " << i << ", count = " << result << endl;
    }
    cout << "Number of combinations is: " << result << endl;
    return 0;
}
/**
 * Закомитьте изменения и отправьте их в свой репозиторий.
 */
