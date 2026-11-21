#include <string>
#include <windows.h>
#include <iostream>
#include <random>
#include <fstream>
#include <climits>
#include <stdexcept>
#include <conio.h>

using namespace std;

const wchar_t* mutexName = L"Local\\RomanovMutex";


struct BankAccount {
	int accountNumber = 10001;
	int balance = 10000;
};

class Bank
{
	BankAccount bankAccount;
	mt19937 mt;
	DWORD processId = GetProcessId(GetCurrentProcess());
	HANDLE hMutex;
public:

	Bank(HANDLE _hMutex)
	{
		hMutex = _hMutex;
	}

	HANDLE GetMutex()
	{
		return hMutex;
	}

	void DefaultValues()
	{

		bankAccount.accountNumber = 10001;
		bankAccount.balance = 10000;
		RewriteFile();

	}

	bool LoadFromFile()
	{
		ifstream file("account.bin", ios::binary);

		if (!file.is_open())
			return false;

		file.seekg(0, ios::end);

		if (file.tellg() != static_cast<streamoff>(2 * sizeof(int)))
			return false;

		file.seekg(0, ios::beg);

		BankAccount temp;

		file.read(reinterpret_cast<char*>(&temp.accountNumber), sizeof(int));
		file.read(reinterpret_cast<char*>(&temp.balance), sizeof(int));
		if (!file.good())
			return false;

		if (temp.accountNumber <= 0 || temp.balance < 0)
			return false;

		bankAccount = temp;
		return true;
	}

	void StartProgram() // need to add a search for a binary file named 'account.bin' and load data from there
	{

		DWORD wait = WaitForSingleObject(hMutex, 3000);

		if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
		{
			throw runtime_error("mutex timed out in StartProgram");
		}

		bool isLoaded = LoadFromFile();

		if (!isLoaded)
		{
			DefaultValues();
		}

		ReleaseMutex(hMutex);
	}


	bool RewriteFile()
	{

		ofstream fileOutput("account.bin", ios::binary | ios::trunc);

		if (fileOutput.is_open())
		{
			fileOutput.write(reinterpret_cast<char*>(&bankAccount.accountNumber), sizeof(bankAccount.accountNumber));
			fileOutput.write(reinterpret_cast<char*>(&bankAccount.balance), sizeof(bankAccount.balance));


			fileOutput.close();

			return true;
		}

		return false;



	}

	void SetSeed(unsigned int seed)
	{
		mt.seed(seed);
	}

	int ViewBalance()
	{
		DWORD wait = WaitForSingleObject(hMutex, 3000);

		if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
		{
			throw runtime_error("mutex timed out in ViewBalance");

		}

		bool isLoaded = LoadFromFile();

		if (!isLoaded)
		{
			DefaultValues();
		}

		int result = bankAccount.balance;

		ReleaseMutex(hMutex);

		return result;
	}

	int AddBalance(int money)
	{
		if (money <= 0)
		{
			return bankAccount.balance;
		}

		DWORD wait = WaitForSingleObject(hMutex, 3000);
		if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) {
			throw runtime_error("mutex timed out in AddBalance");

		}

		bool isLoaded = LoadFromFile();

		if (!isLoaded)
		{
			DefaultValues();
		}

		string logMessage;



		string status = "Success";
		string transaction = "Deposit";

		int oldBalance = bankAccount.balance;

		if (money <= INT_MAX - oldBalance)
		{
			bankAccount.balance = oldBalance + money;

			RewriteFile();


		}
		else
		{
			status = "overflow failure";
		}

		logMessage = to_string(processId) + " | " + to_string(oldBalance) + " | "
			+ to_string(bankAccount.balance) + " | " + transaction + " | "
			+ to_string(money) + " | " + status;

		Log(logMessage.c_str());

		int result = bankAccount.balance;
		ReleaseMutex(hMutex);
		return result;
	}

	int WithdrawBalance(int money)
	{
		if (money <= 0)
		{
			return bankAccount.balance;
		}

		DWORD wait = WaitForSingleObject(hMutex, 3000);
		if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED) {
			throw runtime_error("mutex timed out in WithdrawBalance");

		}

		bool isLoaded = LoadFromFile();

		if (!isLoaded)
		{
			DefaultValues();
		}

		string logMessage;


		string status = "Success";
		string transaction = "Withdrawal";

		int oldBalance = bankAccount.balance;

		if (money <= oldBalance)
		{
			bankAccount.balance = oldBalance - money;

			RewriteFile();


		}
		else
		{
			status = "failure insufficient funds";
		}

		logMessage = to_string(processId) + " | " + to_string(oldBalance) + " | "
			+ to_string(bankAccount.balance) + " | " + transaction + " | "
			+ to_string(money) + " | " + status;

		Log(logMessage.c_str());

		int result = bankAccount.balance;
		ReleaseMutex(hMutex);
		return result;
	}

	void Log(const char* chars)
	{

		ofstream fileOutput("operations.log", ios::app);

		if (fileOutput.is_open())
		{

			fileOutput << chars << "\n";

			cout << chars << "\n";


			fileOutput.close();
		}

	}

	void RandomTransactions(int count)
	{
		if (count <=0)
		{
			return;
		}
		for (int i = 0; i < count; ++i)
		{
			int num = mt() % 2;
			int money = mt() % 1001;
			int delay = mt() % 501;

			if (delay < 100) delay = 100;
			if (money < 100) money = 100;

			Sleep(delay);

			if (num == 1)
				WithdrawBalance(money);
			else
				AddBalance(money);
		}
	}
};

int main()
{
	HANDLE hMutex = CreateMutexW(nullptr, FALSE, mutexName);

	if (hMutex == nullptr)
	{
		return -1;

	}

	try
	{

		random_device rd;

		Bank bank(hMutex);

		bank.SetSeed(rd());



		bank.StartProgram();

		while (true)
		{


			string str;
			std::cout << "1 - View current balance\n2 - Deposit\n3 - Withdraw\n4 - Random transactions\n0 - Exit\n" << std::endl;
			std::cin >> str;

			int choice;
			int value;

			try
			{
				choice = std::stoi(str);
			}
			catch (const std::exception&)
			{
				throw std::invalid_argument("incorrect data type");

			}

			switch (choice)
			{
			case 1:
				cout << bank.ViewBalance() << " roubles" << endl;

				break;
			case 2:
				std::cin >> str;
				try
				{
					value = std::stoi(str);
				}
				catch (const std::exception&)
				{
					throw std::invalid_argument("incorrect data type");

				}

				bank.AddBalance(value);
				break;
			case 3:
				std::cin >> str;
				try
				{
					value = std::stoi(str);
				}
				catch (const std::exception&)
				{
					throw std::invalid_argument("incorrect data type");

				}

				bank.WithdrawBalance(value);
				break;
			case 4:
				bank.RandomTransactions(20);
				break;
			case 0:
				CloseHandle(hMutex);
				return 0;
			default:
				break;
			}
		}



	}
	catch (const std::exception& e)
	{
		cerr << e.what() << endl;

	}

	CloseHandle(hMutex);

	return 1;
}