#include <iostream>
using namespace std;

class Factorial
{
private:
    int num;
    int factorial = 1;

public:
    void calculateFactorial();
    void show();
};

void
Factorial::calculateFactorial()
{
    cout << "Enter a number : " << endl;
    cin >> num;

    if (num == 0 || num == 1)
    {
        factorial = 1;
    }
    else
    {
        while (num > 1)
        {
            factorial = factorial * num;
            num--;
        }
    }
}

void Factorial::show()
{
    cout << "Factorial : " << factorial << endl;
}

int main()
{
    Factorial factorial;
    factorial.calculateFactorial();
    factorial.show();
}
