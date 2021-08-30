#include <iostream>
#include <numeric>
#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include <future>


// подсчёт количества строк в файле
size_t countingLines(std::string path)
{
	std::string buffer;
	std::ifstream file(path);

	size_t i = 0;

	if (!file.is_open())
	{   // если файл не открылся - сообщаем об этом 
		std::cout << "Error open file " + path + "!" << std::endl;
		return 0;  // и возвращаем ноль прочитанных строк
	}
	else
		for (; getline(file, buffer); i++); // читаем построчно, пока есть что читать, и 
	                                        // инкрементируем счётчик, 
	file.close();

	return i;                               // который потом возвращаем.
}

/*
Понадобится вспомогательная функция file_info. Она принимает ссылку на
объект directory_entry и извлекает из нее путь, а также объект file_status
(с помощью функции status), который содержит тип файла. 
*/
static std::filesystem::path file_info(const std::filesystem::directory_entry &entry)
{
	const auto fs(status(entry));
	return entry.path();
}

int main(int argc, char* argv[])
{
	/*
	возьмем текущий каталог ".". Затем проверим, существует ли данный
каталог. При его отсутствии мы не можем создать список файлов.
	*/
	std::filesystem::path dir{ argc > 1 ? argv[1] : "." };
	if (!exists(dir)) {
		std::cout << "Path " << dir << " does not exist.\n";
		return 1;
	}

	// сюда будем складывать пути до файла
	std::vector<std::filesystem::path> pathCollection;

	// здесь будем хранить значения количества строк,
	// прочитанных из файла
	std::vector<size_t> collSum;

	// итератор для обхода директории
	auto directoryIterator = std::filesystem::directory_iterator{ dir };
	
	// максимальное количество потоков (ограниченное "железом")
	size_t maxThreads = std::thread::hardware_concurrency() - 1;

	// здесь будем хранить результат подсчёта строк из файла	
	std::vector<std::future<size_t>> futureObjectCollection;
	
	// обход директории и заполнение коллекции путей до файлов
	std::transform(std::filesystem::directory_iterator{ dir }, {},
		back_inserter(pathCollection), file_info);

	// введём временную переменную для удобства 
	std::string tempFileName = "";

	// по-хорошему, для решения задачи параллельного подсчёта количества строк в файлах
	// нужно использовать пул потоков, но я их ещё не изучал,
	// поэтому попробую сделать собтвенный аналог, 
	// используя объекты std::future, получаемые из std::async
	while (true)
	{
		for (; true;)
		{
			// если обходить больше нечего (файлы закончились)
			// выходим из цикла
			if (directoryIterator == std::filesystem::directory_iterator{})
				break;
			
			// получаем название файла
			tempFileName = directoryIterator->path().filename().generic_string();

			// если размер коллекции с будущими объетами, содержащими 
			// результат подсчёта строк в файлах
			// равен количеству "разрешённых" потоков
			if (futureObjectCollection.size() == maxThreads)
				break; // то выходим из цикла

			// нам нужно читать только текстовые файлы
			// поэтому сделаем соответствующую проверку
			if (tempFileName.find(".txt") != std::string::npos)
				// отправляем файлы в "параллельную" обработку
				futureObjectCollection.push_back(std::async(std::launch::async, countingLines, tempFileName));			
			
			// инкрементируем итератор
			++directoryIterator;
		}
		
		for (size_t i = 0; i < futureObjectCollection.size(); i++)
			// получаем значения количества строк
			collSum.push_back(futureObjectCollection[i].get());			
		
		futureObjectCollection.clear();

		// если обходить больше нечего (файлы закончились)
		// выходим из цикла
		if (directoryIterator == std::filesystem::directory_iterator{})
			break;
	}

	// выводим результат
	std::cout << "the number of lines in the text files of this directory: " 
		      << std::accumulate(collSum.begin(), collSum.end(), 0) << std::endl;	   

	system("pause>nul");
	return 0;
}
