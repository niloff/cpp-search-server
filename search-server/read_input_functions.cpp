#include "read_input_functions.h"

using namespace std;

/**
 * Получить строку введённую через консоль
 */
std::string ReadInput::ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
/**
 * Получить число введённое через консоль
 */
int ReadInput::ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
