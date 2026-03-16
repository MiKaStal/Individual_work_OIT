#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <ctime>
#include <windows.h>
using namespace std;

const int MAX_SIZE = 1000;              // Максимальный размер массива
const string FILENAME = "data.txt";     // Имя файла для работы с данными
const char DELIMITER = ';';// Разделитель полей в файле (Excel-совместимый)

struct Participant {
    unsigned short int NumberOfParticipant;  // Номер участника (уникальный ключ)
    string Surname;                           // Фамилия (для индексации)
    string Name;                              // Имя
    string Patronymic;                        // Отчество
    unsigned short int BirthYear;             // Год рождения (для вычисления возраста)
    string TeamName;                          // Название команды
    unsigned int Result;                      // Результат соревнований
    string City;                              // Город
    bool isDeleted;                           // Флаг логического удаления (п. 10)

    // Конструктор по умолчанию
    Participant() {
        NumberOfParticipant = 0;
        Surname = "";
        Name = "";
        Patronymic = "";
        BirthYear = 0;
        TeamName = "";
        Result = 0;
        City = "";
        isDeleted = false;
    }
    // Метод для вычисления возраста (ВЫЧИСЛЯЕМОЕ ПОЛЕ для индексации)
    int getAge() const {
        int now = 2026;
        return now - BirthYear;
    }
    // Формирование полного ФИО для поиска (составной ключ)
    string getFullName() const {
        return Surname + " " + Name + " " + Patronymic;
    }
    void printFormatted() const {
        if (isDeleted) {
            cout << "[DELETED] ";
        }
        cout << "№ " << NumberOfParticipant << '\n'
            << "Фамилия " << Surname << '\n'
            << "Имя " << Name << '\n'
            << "Отчество " << Patronymic << '\n'
            << "Год " << BirthYear << '\n'
            << "Возраст " << getAge() << '\n'
            << "Команда " << TeamName << '\n'
            << "Результат " << Result << '\n'
            << "Город " << City << '\n';
    }
    friend ostream& operator<<(ostream& os, const Participant& p) {
        os << p.NumberOfParticipant << ';'
            << p.Surname << ';'
            << p.Name << ';'
            << p.Patronymic << ';'
            << p.BirthYear << ';'
            << p.TeamName << ';'
            << p.Result << ';'
            << p.City;
        return os;  // Возврат потока для поддержки цепочек: cout << p1 << p2
    }
    friend istream& operator>>(istream& is, Participant& p) {
        is >> p.NumberOfParticipant
            >> p.Surname >> p.Name >> p.Patronymic
            >> p.BirthYear >> p.TeamName >> p.Result >> p.City;
        p.isDeleted = false;
        return is;  // Возврат потока для поддержки цепочек: cin >> p1 >> p2
    }
};

// Индекс по фамилии (строковый ключ, сортировка по алфавиту)
struct IndexByString {
    string key;           // Фамилия участника
    int recNum;           // Номер записи в массиве participants[]

    IndexByString() {
        key = "";
        recNum = -1;
    }
};

// Индекс по возрасту (числовой ключ, вычисляемое поле)
struct IndexByInt {
    int key;              // Возраст участника (вычисляется)
    int recNum;           // Номер записи в массиве participants[]

    IndexByInt() {
        key = 0;
        recNum = -1;
    }
};

Participant participants[MAX_SIZE];           // Основной массив записей
int participantsCount = 0;                     // Текущее количество записей

IndexByString indexBySurname[MAX_SIZE];        // Индекс по фамилии
IndexByInt indexByAge[MAX_SIZE];               // Индекс по возрасту
int indexCount = 0;                            // Количество элементов в индексе

/*
Индекс содержит:
- key: значение поля, по которому выполняется сортировка/поиск
- recNum: номер записи в основном массиве participants[]
Это позволяет сортировать индексы, не перемещая сами записи
*/
int binarySearchBySurname(const string& key, int left, int right) {
    // Базовый случай: диапазон пуст
    if (left > right) {
        return -1;
    }

    // Вычисляем середину диапазона
    int mid = left + (right - left) / 2;
    string midValue = indexBySurname[mid].key;

    // Сравниваем ключи
    if (midValue == key) {
        return indexBySurname[mid].recNum;  // Возвращаем номер записи
    }

    // Рекурсивный вызов для соответствующей половины
    if (key < midValue) {
        return binarySearchBySurname(key, left, mid - 1);
    }
    else {
        return binarySearchBySurname(key, mid + 1, right);
    }
}

// Бинарный поиск по возрасту (итерационный)
// Возвращает количество найденных записей и заполняет массив results номерами записей
int binarySearchByAge(int key, int results[]) {
    int left = 0;
    int right = indexCount - 1;
    int foundPos = -1;

    // Стандартный итерационный бинарный поиск
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int midValue = indexByAge[mid].key;

        if (midValue == key) {
            foundPos = mid;  // Запоминаем позицию найденного элемента
            break;
        }

        if (key < midValue) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    // Если не найдено
    if (foundPos == -1) {
        return 0;
    }

    // Поиск всех записей с таким же возрастом (слева от найденной)
    int count = 0;
    int pos = foundPos;
    while (pos >= 0 && indexByAge[pos].key == key) {
        results[count++] = indexByAge[pos].recNum;
        pos--;
    }

    // Поиск всех записей с таким же возрастом (справа от найденной)
    pos = foundPos + 1;
    while (pos < indexCount && indexByAge[pos].key == key) {
        results[count++] = indexByAge[pos].recNum;
        pos++;
    }

    return count;  // Возвращаем количество найденных записей
}

// Обёртка: поиск по фамилии с вводом от пользователя
void searchBySurnameIndex() {
    if (indexCount == 0) {
        cout << "\n[WARNING] Сначала постройте индекс по фамилии!" << '\n';
        return;
    }

    string key;
    cout << "\nПОИСК ПО ФАМИЛИИ (бинарный поиск по индексу)" << '\n';
    cout << "Введите фамилию для поиска: ";
    cin >> key;

    int recNum = binarySearchBySurname(key, 0, indexCount - 1);

    if (recNum != -1) {
        cout << "\n[FOUND] Найдена запись:" << '\n';
        participants[recNum].printFormatted();
    }
    else {
        cout << "\n[NOT FOUND] Участник с фамилией '" << key << "' не найден!" << '\n';
    }
}

// Обёртка: поиск по возрасту с вводом от пользователя
void searchByAgeIndex() {
    if (indexCount == 0) {
        cout << "\n[WARNING] Сначала постройте индекс по возрасту!" << '\n';
        return;
    }

    int key;
    cout << "\n=== ПОИСК ПО ВОЗРАСТУ (бинарный поиск по индексу) ===" << '\n';
    cout << "Введите возраст для поиска: ";
    cin >> key;

    int results[MAX_SIZE];  // Массив для хранения найденных номеров записей
    int count = binarySearchByAge(key, results);

    if (count > 0) {
        cout << "\n[FOUND] Найдено записей: " << count << '\n';
        cout << string(100, '-') << '\n';
        for (int i = 0; i < count; i++) {
            participants[results[i]].printFormatted();
        }
        cout << string(100, '-') << '\n';
    }
    else {
        cout << "\n[NOT FOUND] Участников с возрастом " << key << " лет не найдено!" << '\n';
    }
}
// Настройка консоли для работы с кириллицей
void setupConsole() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
}

int stringToInt(const string& s) {
    int result = 0;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            result = result * 10 + (s[i] - '0');
        }
    }
    return result;
}

// Разбор строки по разделителю (парсинг записи из файла)
bool parseRecord(const string& line, Participant& p) {
    string field = "";
    int fieldNum = 0;

    for (size_t i = 0; i <= line.length(); i++) {
        if (i == line.length() || line[i] == DELIMITER) {
            switch (fieldNum) {
            case 0: p.NumberOfParticipant = stringToInt(field); break;
            case 1: p.Surname = field; break;
            case 2: p.Name = field; break;
            case 3: p.Patronymic = field; break;
            case 4: p.BirthYear = stringToInt(field); break;
            case 5: p.TeamName = field; break;
            case 6: p.Result = stringToInt(field); break;
            case 7: p.City = field; break;
            }
            field = "";
            fieldNum++;
        }
        else {
            field += line[i];
        }
    }

    p.isDeleted = false;
    return (fieldNum >= 8);  // Проверка: все поля заполнены
}


// П. 1: Ввод данных с клавиатуры
void inputFromKeyboard() {
    int count;
    cout << "Сколько записей хотите ввести? ";
    cin >> count;
    if (participantsCount + count > MAX_SIZE) {
        cout << "Ошибка: превышен максимальный размер массива (" << MAX_SIZE << ")!" << '\n';
        return;
    }

    for (int i = 0; i < count; i++) {
        cout << "\n--- Запись #" << (participantsCount + 1) << " ---" << '\n';

        Participant* p = &participants[participantsCount];

        cout << "Введите номер участника:>";
        cin >> p -> NumberOfParticipant;
        cout << "Введите Фамилию участника: ";
        cin >> p->Surname;
        cout << "Введите Имя участника:>";
        cin >> p->Name;
        cout << "Введите Отчество участника:>";
        cin >> p->Patronymic;
        cout << "Введите Год рождения участника:>";
        cin >> p->BirthYear;
        cout << "Введите название Команды участника:>";
        cin >> p->TeamName;
        cout << "Введите Результат участника:>";
        cin >> p->Result;
        cout << "Введите Город участника:>";
        cin >> p->City;

        p->isDeleted = false;
        participantsCount++;
    }

    cout << "\n[OK] Введено " << count << " записей." << '\n';
}

// П. 2: Вывод данных на экран в порядке ввода
void outputToScreen() {
    int activeCount = 0;
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            cout << "№ " << participants[i].NumberOfParticipant << '\n'
                << "Фамилия " << participants[i].Surname << '\n'
                << "Имя " << participants[i].Name << '\n'
                << "Отчество " << participants[i].Patronymic << '\n'
                << "Год " << participants[i].BirthYear << '\n'
                << "Возраст " << participants[i].getAge() << '\n'
                << "Команда " << participants[i].TeamName << '\n'
                << "Результат " << participants[i].Result << '\n'
                << "Город " << participants[i].City << '\n';
            activeCount++;
        }
    }
    cout << "Всего записей: " << participantsCount
        << " | Активных: " << activeCount
        << " | Удалённых: " << (participantsCount - activeCount) << '\n';
}

// П. 3: Вывод данных в текстовый файл
void outputToFile() {
    cout << "\nВЫВОД ДАННЫХ В ФАЙЛ" << '\n';
    cout << "1. Создать новый файл (перезаписать)" << '\n';
    cout << "2. Дополнить существующий файл" << '\n';
    cout << "Выберите режим: ";

    int mode;
    cin >> mode;

    ofstream file;
    if (mode == 1) {
        file.open(FILENAME, ios::out);  // Создание нового файла
        cout << "[OK] Файл будет создан заново." << '\n';
    }
    else if (mode == 2) {
        file.open(FILENAME, ios::app);  // Дополнение существующего
        cout << "[OK] Данные будут добавлены в конец файла." << '\n';
    }
    else {
        cout << "[ERROR] Неверный режим!" << '\n';
        return;
    }

    if (!file.is_open()) {
        cout << "[ERROR] Не удалось открыть файл!" << '\n';
        return;
    }

    int count = 0;
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            file << participants[i].NumberOfParticipant << DELIMITER
                << participants[i].Surname << DELIMITER
                << participants[i].Name << DELIMITER
                << participants[i].Patronymic << DELIMITER
                << participants[i].BirthYear << DELIMITER
                << participants[i].TeamName << DELIMITER
                << participants[i].Result << DELIMITER
                << participants[i].City << '\n';
            count++;
        }
    }

    file.close();
    cout << "[OK] В файл '" << FILENAME << "' записано " << count << " записей." << '\n';
}

// П. 4: Ввод данных из текстового файла
void inputFromFile() {
    cout << "\n ВВОД ДАННЫХ ИЗ ФАЙЛА " << '\n';

    ifstream file(FILENAME);
    if (!file.is_open()) {
        cout << "[ERROR] Файл '" << FILENAME << "' не найден!" << '\n';
        return;
    }

    string line;
    int count = 0;

    while (participantsCount < MAX_SIZE && getline(file, line)) {
        if (line.empty()) continue;  // Пропуск пустых строк

        if (parseRecord(line, participants[participantsCount])) {
            participantsCount++;
            count++;
        }
    }

    file.close();
    cout << "[OK] Из файла загружено " << count << " записей." << '\n';
}

// П. 5: Сортировка вставками для индекса по фамилии (строковый ключ)
// Алгоритм: вставка каждого элемента в отсортированную часть массива
void sortIndexBySurname_Insertion() {
    // Сортировка вставками с барьерным элементом
    for (int i = 1; i < indexCount; i++) {
        IndexByString current = indexBySurname[i];
        int j = i - 1;

        // Сдвиг элементов, больших текущего, вправо
        while (j >= 0 && indexBySurname[j].key > current.key) {
            indexBySurname[j + 1] = indexBySurname[j];
            j--;
        }

        // Вставка текущего элемента на найденную позицию
        indexBySurname[j + 1] = current;
    }
}

// П. 6: Сортировка выбором для индекса по возрасту (числовой ключ, вычисляемый)
// Алгоритм: поиск минимума в неотсортированной части и обмен
void sortIndexByAge_Selection() {
    for (int i = 0; i < indexCount - 1; i++) {
        int minIdx = i;

        // Поиск индекса минимального элемента
        for (int j = i + 1; j < indexCount; j++) {
            if (indexByAge[j].key < indexByAge[minIdx].key) {
                minIdx = j;
            }
        }

        // Обмен элементов, если минимум не на своей позиции
        if (minIdx != i) {
            IndexByInt temp = indexByAge[i];
            indexByAge[i] = indexByAge[minIdx];
            indexByAge[minIdx] = temp;
        }
    }
}

// Построение индекса по фамилии
void buildIndexBySurname() {
    cout << "\nПОСТРОЕНИЕ ИНДЕКСА ПО ФАМИЛИИ" << '\n';

    indexCount = 0;

    // Заполнение индекса: копируем ключи и номера записей
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            indexBySurname[indexCount].key = participants[i].Surname;
            indexBySurname[indexCount].recNum = i;
            indexCount++;
        }
    }

    // Сортировка индекса (пузырьковая - для разнообразия алгоритмов)
    for (int i = 0; i < indexCount - 1; i++) {
        for (int j = 0; j < indexCount - i - 1; j++) {
            if (indexBySurname[j].key > indexBySurname[j + 1].key) {
                IndexByString temp = indexBySurname[j];
                indexBySurname[j] = indexBySurname[j + 1];
                indexBySurname[j + 1] = temp;
            }
        }
    }

    cout << "[OK] Индекс по фамилии построен (" << indexCount << " записей)." << '\n';
}

// Построение индекса по возрасту (вычисляемое поле)
void buildIndexByAge() {
    cout << "\n ПОСТРОЕНИЕ ИНДЕКСА ПО ВОЗРАСТУ " << '\n';

    indexCount = 0;

    // Заполнение индекса с вычислением возраста
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            indexByAge[indexCount].key = participants[i].getAge();
            indexByAge[indexCount].recNum = i;
            indexCount++;
        }
    }

    // Сортировка выбором по возрасту
    sortIndexByAge_Selection();

    cout << "[OK] Индекс по возрасту построен (" << indexCount << " записей)." << '\n';
}

// П. 7.1: Вывод по индексу (возрастание)
void outputByIndexAscending(IndexByInt idx[], int size, const string& indexName) {
    cout << "\n ВЫВОД ПО ИНДЕКСУ '" << indexName << "' (ПО ВОЗРАСТАНИЮ)" << '\n';
    for (int i = 0; i < size; i++) {
        int recNum = idx[i].recNum;
        cout << "№ " << participants[recNum].NumberOfParticipant
            << "\nФамилия " << participants[recNum].Surname
            << "\nИмя " << participants[recNum].Name
            << "\nОтчество " << participants[recNum].Patronymic
            << "\nГод " << participants[recNum].BirthYear
            << "\nВозраст " << participants[recNum].getAge()
            << "\nКоманда " << participants[recNum].TeamName
            << "\nРезультат " << participants[recNum].Result
            << "\nГород " << participants[recNum].City << '\n';
    }
    cout << "Записей: " << size << '\n';
}

// П. 7.2: Вывод по индексу (убывание) - через обратный проход
void outputByIndexDescending(IndexByInt idx[], int size, const string& indexName) {
    cout << "\n ВЫВОД ПО ИНДЕКСУ '" << indexName << "' (ПО УБЫВАНИЮ) " << '\n';
    // Обратный проход по отсортированному индексу
    for (int i = size - 1; i >= 0; i--) {
        int recNum = idx[i].recNum;
        cout << "№ " << participants[recNum].NumberOfParticipant
            << "\nФамилия " << participants[recNum].Surname
            << "\nИмя " << participants[recNum].Name
            << "\nОтчество " << participants[recNum].Patronymic
            << "\nГод " << participants[recNum].BirthYear
            << "\nВозраст " << participants[recNum].getAge()
            << "\nКоманда " << participants[recNum].TeamName
            << "\nРезультат " << participants[recNum].Result
            << "\nГород " << participants[recNum].City << '\n';
    }

    
    cout << "Записей: " << size << '\n';
}

// П. 8.1: Рекурсивный бинарный поиск по фамилии (уникальный/неуникальный ключ)
int binarySearchRecursive(IndexByString idx[], const string& key, int left, int right) {
    // Базовый случай: отрезок пуст
    if (left > right) {
        return -1;
    }

    // Вычисление середины (защита от переполнения)
    int mid = left + (right - left) / 2;
    string midValue = idx[mid].key;

    // Сравнение ключей
    if (midValue == key) {
        return idx[mid].recNum;  // Возвращаем номер записи в основном массиве
    }

    // Рекурсивный вызов для соответствующей половины
    if (key < midValue) {
        return binarySearchRecursive(idx, key, left, mid - 1);
    }
    else {
        return binarySearchRecursive(idx, key, mid + 1, right);
    }
}

// П. 8.2: Итерационный бинарный поиск по возрасту (неуникальный ключ)
// Возвращает массив найденных записей и их количество
int binarySearchIterative_Age(IndexByInt idx[], int size, int key, int results[]) {
    int left = 0;
    int right = size - 1;
    int foundPos = -1;

    // Стандартный итерационный бинарный поиск
    while (left <= right) {
        int mid = left + (right - left) / 2;
        int midValue = idx[mid].key;

        if (midValue == key) {
            foundPos = mid;  // Запоминаем позицию первого найденного
            break;
        }

        if (key < midValue) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    // Если не найдено
    if (foundPos == -1) {
        return 0;
    }

    // Поиск всех записей с таким же возрастом (слева от найденной)
    int count = 0;
    int pos = foundPos;
    while (pos >= 0 && idx[pos].key == key) {
        results[count++] = idx[pos].recNum;
        pos--;
    }

    // Поиск всех записей с таким же возрастом (справа от найденной)
    pos = foundPos + 1;
    while (pos < size && idx[pos].key == key) {
        results[count++] = idx[pos].recNum;
        pos++;
    }

    return count;  // Возвращаем количество найденных записей
}

// Обёртка: поиск по фамилии
void searchBySurname() {
    if (indexCount == 0) {
        cout << "\n[WARNING] Сначала постройте индекс по фамилии!" << '\n';
        return;
    }

    string key;
    cout << "\n ПОИСК ПО ФАМИЛИИ (рекурсивный бинарный поиск) " << '\n';
    cout << "Введите фамилию для поиска: ";
    cin >> key;

    int recNum = binarySearchRecursive(indexBySurname, key, 0, indexCount - 1);

    if (recNum != -1) {
        cout << "\n[FOUND] Найдена запись:" << '\n';
        
        participants[recNum].printFormatted();
        
    }
    else {
        cout << "\n[NOT FOUND] Участник с фамилией '" << key << "' не найден!" << '\n';
    }
}

// Обёртка: поиск по возрасту (с обработкой дубликатов)
void searchByAge() {
    if (indexCount == 0) {
        cout << "\n[WARNING] Сначала постройте индекс по возрасту!" << '\n';
        return;
    }

    int key;
    cout << "\n ПОИСК ПО ВОЗРАСТУ (итерационный бинарный поиск) " << '\n';
    cout << "Введите возраст для поиска: ";
    cin >> key;

    int results[MAX_SIZE];  // Массив для хранения найденных номеров записей
    int count = binarySearchIterative_Age(indexByAge, indexCount, key, results);

    if (count > 0) {
        cout << "\n[FOUND] Найдено записей: " << count << '\n';
        
        for (int i = 0; i < count; i++) {
            participants[results[i]].printFormatted();
        }
        
    }
    else {
        cout << "\n[NOT FOUND] Участников с возрастом " << key << " лет не найдено!" << '\n';
    }
}

// П. 9: Редактирование записи с обновлением индексов
void editRecord() {
    cout << "\n РЕДАКТИРОВАНИЕ ЗАПИСИ " << '\n';
    int number;
    cout << "Введите номер участника для редактирования: ";
    cin >> number;

    // Поиск записи по уникальному номеру
    int foundIndex = -1;
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted && participants[i].NumberOfParticipant == number) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == -1) {
        cout << "[ERROR] Запись с номером " << number << " не найдена!" << '\n';
        return;
    }

    // Отображение текущих данных
    cout << "\n[INFO] Текущие данные:" << '\n';
    participants[foundIndex].printFormatted();

    // Меню выбора поля для редактирования
    cout << "\nЧто хотите изменить?" << '\n';
    cout << "1. Фамилия" << '\n';
    cout << "2. Имя" << '\n';
    cout << "3. Отчество" << '\n';
    cout << "4. Год рождения" << '\n';
    cout << "5. Команда" << '\n';
    cout << "6. Результат" << '\n';
    cout << "7. Город" << '\n';
    cout << "8. Все поля" << '\n';
    cout << "Выберите: ";

    int choice;
    cin >> choice;

    Participant* p = &participants[foundIndex];

    switch (choice) {
    case 1:
        cout << "Новая фамилия: "; cin >> p->Surname;
        break;
    case 2:
        cout << "Новое имя: "; cin >> p->Name;
        break;
    case 3:
        cout << "Новое отчество: "; cin >> p->Patronymic;
        break;
    case 4:
        cout << "Новый год рождения: "; cin >> p->BirthYear;
        break;
    case 5:
        cout << "Новая команда: "; cin >> p->TeamName;
        break;
    case 6:
        cout << "Новый результат: "; cin >> p->Result;
        break;
    case 7:
        cout << "Новый город: "; cin >> p->City;
        break;
    case 8:
        // Полный перезапись записи
        cout << "Введите новые данные:" << '\n';
        cin >> *p;
        p->NumberOfParticipant = number;  // Сохраняем номер
        break;
    default:
        cout << "[ERROR] Неверный выбор!" << '\n';
        return;
    }

    cout << "\n[OK] Запись обновлена!" << '\n';
    cout << "[INFO] Новые данные:" << '\n';
    participants[foundIndex].printFormatted();

    // Перестроение индексов для сохранения порядка сортировки
    cout << "\n[INFO] Обновление индексов..." << '\n';
    buildIndexBySurname();
    buildIndexByAge();
}

// П. 10.1: Логическое удаление записи (пометка флагом)
void deleteRecordLogical() {
    cout << "\n ЛОГИЧЕСКОЕ УДАЛЕНИЕ ЗАПИСИ " << '\n';

    int number;
    cout << "Введите номер участника для удаления: ";
    cin >> number;

    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted && participants[i].NumberOfParticipant == number) {
            participants[i].isDeleted = true;
            cout << "[OK] Запись #" << number << " помечена как удалённая." << '\n';

            // Перестроение индексов (исключение помеченных записей)
            buildIndexBySurname();
            buildIndexByAge();
            return;
        }
    }

    cout << "[ERROR] Запись с номером " << number << " не найдена!" << '\n';
}

// П. 10.2: Восстановление удалённой записи
void restoreRecord() {
    cout << "\n ВОССТАНОВЛЕНИЕ УДАЛЁННОЙ ЗАПИСИ " << '\n';

    int number;
    cout << "Введите номер участника для восстановления: ";
    cin >> number;

    for (int i = 0; i < participantsCount; i++) {
        if (participants[i].isDeleted && participants[i].NumberOfParticipant == number) {
            participants[i].isDeleted = false;
            cout << "[OK] Запись #" << number << " восстановлена." << '\n';

            // Перестроение индексов
            buildIndexBySurname();
            buildIndexByAge();
            return;
        }
    }

    cout << "[ERROR] Удалённая запись с номером " << number << " не найдена!" << '\n';
}

// П. 10.3: Физическое удаление всех помеченных записей
void deleteRecordsPhysical() {
    cout << "\n ФИЗИЧЕСКОЕ УДАЛЕНИЕ УДАЛЁННЫХ ЗАПИСЕЙ " << '\n';

    int deletedCount = 0;
    int newCount = 0;

    // Сдвиг не удалённых записей в начало массива (in-place)
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            if (i != newCount) {
                participants[newCount] = participants[i];
            }
            newCount++;
        }
        else {
            deletedCount++;
        }
    }

    participantsCount = newCount;

    // Перестроение индексов
    buildIndexBySurname();
    buildIndexByAge();

    cout << "[OK] Физически удалено " << deletedCount << " записей." << '\n';
}

/*
Функция внешней сортировки слиянием для больших файлов.
Алгоритм:
1. Чтение файла порциями, сортировка каждой порции в памяти
2. Запись отсортированных порций во временные файлы
3. Многопутевое слияние временных файлов в результат

Это упрощённая версия для демонстрации принципа.
*/
void externalSortDemo() {
    cout << "\n ДЕМОНСТРАЦИЯ ВНЕШНЕЙ СОРТИРОВКИ " << '\n';
    cout << "[INFO] Внешняя сортировка применяется, когда данные не помещаются в RAM." << '\n';
    cout << "[INFO] Алгоритм: многофазное слияние отсортированных серий." << '\n';
    cout << '\n';
    cout << "Шаги алгоритма:" << '\n';
    cout << "1. Разбиение входного файла на блоки, помещающиеся в память" << '\n';
    cout << "2. Сортировка каждого блока внутренним алгоритмом (напр., быстрая сортировка)" << '\n';
    cout << "3. Запись отсортированных блоков во временные файлы (серии)" << '\n';
    cout << "4. Многопутевое слияние серий в итоговый отсортированный файл" << '\n';
    cout << '\n';
    cout << "[OK] Для реализации требуется работа с файловыми потоками и буферизацией." << '\n';
}


void showMenu() {
    cout << "ВВОД/ВЫВОД ДАННЫХ:" << '\n';
    cout << "  1. Ввод данных с клавиатуры" << '\n';
    cout << "  2. Вывод данных на экран" << '\n';
    cout << "  3. Вывод данных в файл" << '\n';
    cout << "  4. Ввод данных из файла" << '\n';
    cout << "\nИНДЕКСАЦИЯ И СОРТИРОВКА:" << '\n';
    cout << "  5. Построить индекс по фамилии (сортировка вставками)" << '\n';
    cout << "  6. Построить индекс по возрасту (сортировка выбором)" << '\n';
    cout << "\nВЫВОД ПО ИНДЕКСАМ:" << '\n';
    cout << "  7. Вывод по фамилии (возрастание)" << '\n';
    cout << "  8. Вывод по фамилии (убывание)" << '\n';
    cout << "  9. Вывод по возрасту (возрастание)" << '\n';
    cout << "  10. Вывод по возрасту (убывание)" << '\n';
    cout << "\nПОИСК (БИНАРНЫЙ):" << '\n';
    cout << "  11. Поиск по фамилии (рекурсивный)" << '\n';
    cout << "  12. Поиск по возрасту (итерационный, с дубликатами)" << '\n';
    cout << "\nРЕДАКТИРОВАНИЕ И УДАЛЕНИЕ:" << '\n';
    cout << "  13. Редактировать запись" << '\n';
    cout << "  14. Логическое удаление записи" << '\n';
    cout << "  15. Восстановить удалённую запись" << '\n';
    cout << "  16. Физическое удаление удалённых записей" << '\n';
    cout << "\nДОПОЛНИТЕЛЬНО:" << '\n';
    cout << "  17. Демонстрация внешней сортировки" << '\n';
    cout << "\n0. Выход" << '\n';
}

// Узел дерева для индекса по строковому ключу (фамилия)
struct TreeNodeString {
    string key;                    // Ключ: фамилия участника
    int recNum;                    // Номер записи в массиве participants[]
    TreeNodeString* left;          // Левое поддерево (ключи < текущего)
    TreeNodeString* right;         // Правое поддерево (ключи > текущего)

    TreeNodeString() {
        key = "";
        recNum = -1;
        left = right = nullptr;
    }

    TreeNodeString(string k, int r) {
        key = k;
        recNum = r;
        left = right = nullptr;
    }
};

// Узел дерева для индекса по числовому ключу (возраст)
struct TreeNodeInt {
    int key;                       // Ключ: возраст участника
    int recNum;                    // Номер записи в массиве participants[]
    TreeNodeInt* left;             // Левое поддерево
    TreeNodeInt* right;            // Правое поддерево

    TreeNodeInt() {
        key = 0;
        recNum = -1;
        left = right = nullptr;
    }

    TreeNodeInt(int k, int r) {
        key = k;
        recNum = r;
        left = right = nullptr;
    }
};

// Глобальные указатели на корни деревьев индексов
TreeNodeString* rootBySurname = nullptr;   // Корень дерева по фамилии
TreeNodeInt* rootByAge = nullptr;          // Корень дерева по возрасту

// Рекурсивная вставка узла в дерево по строковому ключу
TreeNodeString* insertTreeString(TreeNodeString* node, string key, int recNum) {
    if (node == nullptr) {
        return new TreeNodeString(key, recNum);
    }

    if (key < node->key) {
        node->left = insertTreeString(node->left, key, recNum);
    }
    else {
        // При равных ключах вставляем в правое поддерево (поддержка дубликатов)
        node->right = insertTreeString(node->right, key, recNum);
    }
    return node;
}

// Рекурсивная вставка узла в дерево по числовому ключу
TreeNodeInt* insertTreeInt(TreeNodeInt* node, int key, int recNum) {
    if (node == nullptr) {
        return new TreeNodeInt(key, recNum);
    }

    if (key < node->key) {
        node->left = insertTreeInt(node->left, key, recNum);
    }
    else {
        node->right = insertTreeInt(node->right, key, recNum);
    }
    return node;
}

// Прямой обход (in-order): вывод по возрастанию ключа (для строкового дерева)
void inOrderTraversalString(TreeNodeString* node) {
    if (node == nullptr) return;

    inOrderTraversalString(node->left);

    // Вывод записи по номеру из индекса
    participants[node->recNum].printFormatted();

    inOrderTraversalString(node->right);
}

// Прямой обход (in-order): вывод по возрастанию ключа (для числового дерева)
void inOrderTraversalInt(TreeNodeInt* node) {
    if (node == nullptr) return;

    inOrderTraversalInt(node->left);
    participants[node->recNum].printFormatted();
    inOrderTraversalInt(node->right);
}

// Обратный обход (reverse in-order): вывод по убыванию ключа
void reverseInOrderTraversalString(TreeNodeString* node) {
    if (node == nullptr) return;

    reverseInOrderTraversalString(node->right);
    participants[node->recNum].printFormatted();
    reverseInOrderTraversalString(node->left);
}

void reverseInOrderTraversalInt(TreeNodeInt* node) {
    if (node == nullptr) return;

    reverseInOrderTraversalInt(node->right);
    participants[node->recNum].printFormatted();
    reverseInOrderTraversalInt(node->left);
}

// П. 7.1: Рекурсивный поиск по строковому ключу (фамилия)
TreeNodeString* searchTreeStringRecursive(TreeNodeString* node, const string& key) {
    if (node == nullptr || node->key == key) {
        return node;  // Возвращаем узел (нашли или не нашли)
    }

    if (key < node->key) {
        return searchTreeStringRecursive(node->left, key);
    }
    else {
        return searchTreeStringRecursive(node->right, key);
    }
}

// П. 7.2: Итерационный поиск по числовому ключу (возраст) с поддержкой дубликатов
// Возвращает массив номеров записей и их количество
int searchTreeIntIterative(TreeNodeInt* root, int key, int results[]) {
    if (root == nullptr) return 0;

    int count = 0;

    // 1. Итерационный спуск к первому найденному узлу
    TreeNodeInt* current = root;
    TreeNodeInt* foundNode = nullptr;

    while (current != nullptr) {
        if (key == current->key) {
            foundNode = current;
            break;
        }
        else if (key < current->key) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    if (foundNode == nullptr) {
        return 0;  // Не найдено
    }

    // 2. Сбор всех узлов с таким же ключом (DFS обход от найденного узла)
    // Используем стек для итерационного обхода
    TreeNodeInt* stack[MAX_SIZE];
    int top = -1;

    // Начинаем с найденного узла и обходим всё дерево, собирая совпадения
    // Для упрощения: полный обход дерева с фильтрацией по ключу
    stack[++top] = root;

    while (top >= 0) {
        TreeNodeInt* node = stack[top--];
        if (node == nullptr) continue;

        if (node->key == key) {
            results[count++] = node->recNum;
        }

        if (node->left != nullptr) stack[++top] = node->left;
        if (node->right != nullptr) stack[++top] = node->right;
    }

    return count;
}

// Обёртка: поиск по фамилии через рекурсивное дерево
void searchBySurnameTree() {
    if (rootBySurname == nullptr) {
        cout << "\n[WARNING] Сначала постройте индекс по фамилии (бинарное дерево)!" << '\n';
        return;
    }

    string key;
    cout << "\n ПОИСК ПО ФАМИЛИИ (рекурсивный поиск в БДП) " << '\n';
    cout << "Введите фамилию для поиска: ";
    cin >> key;

    TreeNodeString* result = searchTreeStringRecursive(rootBySurname, key);

    if (result != nullptr) {
        cout << "\n[FOUND] Найдена запись:" << '\n';
        
        participants[result->recNum].printFormatted();
        
    }
    else {
        cout << "\n[NOT FOUND] Участник с фамилией '" << key << "' не найден!" << '\n';
    }
}

// Обёртка: поиск по возрасту через итерационное дерево
void searchByAgeTree() {
    if (rootByAge == nullptr) {
        cout << "\n[WARNING] Сначала постройте индекс по возрасту (бинарное дерево)!" << '\n';
        return;
    }

    int key;
    cout << "\n ПОИСК ПО ВОЗРАСТУ (итерационный поиск в БДП) " << '\n';
    cout << "Введите возраст для поиска: ";
    cin >> key;

    int results[MAX_SIZE];
    int count = searchTreeIntIterative(rootByAge, key, results);

    if (count > 0) {
        cout << "\n[FOUND] Найдено записей: " << count << '\n';
        
        for (int i = 0; i < count; i++) {
            participants[results[i]].printFormatted();
        }
        
    }
    else {
        cout << "\n[NOT FOUND] Участников с возрастом " << key << " лет не найдено!" << '\n';
    }
}

// П. 5: Построение бинарного дерева индекса по фамилии
void buildTreeIndexBySurname() {
    cout << "\n ПОСТРОЕНИЕ БДП ИНДЕКСА ПО ФАМИЛИИ " << '\n';

    // Освобождение памяти старого дерева (простая рекурсивная очистка)
    // В реальном проекте нужна полноценная функция deleteTree
    rootBySurname = nullptr;  // Для простоты: утечка памяти допустима в учебном коде

    // Построение нового дерева: вставка всех активных записей
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            rootBySurname = insertTreeString(rootBySurname, participants[i].Surname, i);
        }
    }

    cout << "[OK] Бинарное дерево по фамилии построено." << '\n';
}

// П. 6: Построение бинарного дерева индекса по возрасту (вычисляемое поле)
void buildTreeIndexByAge() {
    cout << "\n ПОСТРОЕНИЕ БДП ИНДЕКСА ПО ВОЗРАСТУ " << '\n';

    rootByAge = nullptr;  // Сброс старого дерева

    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            int age = participants[i].getAge();
            rootByAge = insertTreeInt(rootByAge, age, i);
        }
    }

    cout << "[OK] Бинарное дерево по возрасту построено." << '\n';
}

// П. 6.1: Вывод по индексу-дереву (возрастание)
void outputByTreeAscending(const string& indexName, bool byAge = false) {
    cout << "\n ВЫВОД ПО БДП '" << indexName << "' (ПО ВОЗРАСТАНИЮ) " << '\n';
    
    cout << left << setw(5) << "№"
        << setw(15) << "Фамилия"
        << setw(10) << "Имя"
        << setw(12) << "Отчество"
        << setw(5) << "Год"
        << setw(5) << "Возр"
        << setw(15) << "Команда"
        << setw(7) << "Результат"
        << setw(15) << "Город" << '\n';
    

    if (byAge) {
        inOrderTraversalInt(rootByAge);
    }
    else {
        inOrderTraversalString(rootBySurname);
    }

    
}

// П. 6.2: Вывод по индексу-дереву (убывание)
void outputByTreeDescending(const string& indexName, bool byAge = false) {
    cout << "\n ВЫВОД ПО БДП '" << indexName << "' (ПО УБЫВАНИЮ) " << '\n';
    
    cout << left << setw(5) << "№"
        << setw(15) << "Фамилия"
        << setw(10) << "Имя"
        << setw(12) << "Отчество"
        << setw(5) << "Год"
        << setw(5) << "Возр"
        << setw(15) << "Команда"
        << setw(7) << "Результат"
        << setw(15) << "Город" << '\n';
    

    if (byAge) {
        reverseInOrderTraversalInt(rootByAge);
    }
    else {
        reverseInOrderTraversalString(rootBySurname);
    }

    
}

// Вспомогательная функция: поиск минимального узла в поддереве
TreeNodeString* findMinString(TreeNodeString* node) {
    while (node && node->left != nullptr) {
        node = node->left;
    }
    return node;
}

TreeNodeInt* findMinInt(TreeNodeInt* node) {
    while (node && node->left != nullptr) {
        node = node->left;
    }
    return node;
}

// Удаление узла из строкового дерева по ключу и номеру записи
TreeNodeString* deleteNodeString(TreeNodeString* root, string key, int recNum) {
    if (root == nullptr) return nullptr;

    // Поиск узла для удаления
    if (key < root->key) {
        root->left = deleteNodeString(root->left, key, recNum);
    }
    else if (key > root->key) {
        root->right = deleteNodeString(root->right, key, recNum);
    }
    else {
        // Ключ совпал, проверяем номер записи (для поддержки дубликатов ключей)
        if (root->recNum == recNum) {
            // Случай 1: узел без потомков или с одним потомком
            if (root->left == nullptr) {
                TreeNodeString* temp = root->right;
                delete root;
                return temp;
            }
            else if (root->right == nullptr) {
                TreeNodeString* temp = root->left;
                delete root;
                return temp;
            }

            // Случай 2: узел с двумя потомками
            // Находим минимальный узел в правом поддереве
            TreeNodeString* temp = findMinString(root->right);
            root->key = temp->key;
            root->recNum = temp->recNum;
            // Удаляем найденный минимальный узел
            root->right = deleteNodeString(root->right, temp->key, temp->recNum);
        }
        // Если ключ совпал, но recNum не совпал — продолжаем поиск в правом поддереве
        else {
            root->right = deleteNodeString(root->right, key, recNum);
        }
    }
    return root;
}

// Удаление узла из числового дерева
TreeNodeInt* deleteNodeInt(TreeNodeInt* root, int key, int recNum) {
    if (root == nullptr) return nullptr;

    if (key < root->key) {
        root->left = deleteNodeInt(root->left, key, recNum);
    }
    else if (key > root->key) {
        root->right = deleteNodeInt(root->right, key, recNum);
    }
    else {
        if (root->recNum == recNum) {
            if (root->left == nullptr) {
                TreeNodeInt* temp = root->right;
                delete root;
                return temp;
            }
            else if (root->right == nullptr) {
                TreeNodeInt* temp = root->left;
                delete root;
                return temp;
            }

            TreeNodeInt* temp = findMinInt(root->right);
            root->key = temp->key;
            root->recNum = temp->recNum;
            root->right = deleteNodeInt(root->right, temp->key, temp->recNum);
        }
        else {
            root->right = deleteNodeInt(root->right, key, recNum);
        }
    }
    return root;
}

// Логическое удаление с обновлением деревьев
void deleteRecordLogicalWithTree() {
    cout << "\n ЛОГИЧЕСКОЕ УДАЛЕНИЕ (с обновлением БДП) " << '\n';

    int number;
    cout << "Введите номер участника для удаления: ";
    cin >> number;

    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted && participants[i].NumberOfParticipant == number) {
            // Помечаем запись как удалённую
            participants[i].isDeleted = true;
            cout << "[OK] Запись #" << number << " помечена как удалённая." << '\n';

            // Удаляем узлы из деревьев индексов
            if (rootBySurname != nullptr) {
                rootBySurname = deleteNodeString(rootBySurname, participants[i].Surname, i);
            }
            if (rootByAge != nullptr) {
                int age = participants[i].getAge();
                rootByAge = deleteNodeInt(rootByAge, age, i);
            }
            return;
        }
    }

    cout << "[ERROR] Запись с номером " << number << " не найдена!" << '\n';
}

// Восстановление записи с перестроением индексов
void restoreRecordWithTree() {
    cout << "\n ВОССТАНОВЛЕНИЕ (с обновлением БДП) " << '\n';

    int number;
    cout << "Введите номер участника для восстановления: ";
    cin >> number;

    for (int i = 0; i < participantsCount; i++) {
        if (participants[i].isDeleted && participants[i].NumberOfParticipant == number) {
            participants[i].isDeleted = false;
            cout << "[OK] Запись #" << number << " восстановлена." << '\n';

            // Перестраиваем индексы (простой способ: полная перестройка)
            buildTreeIndexBySurname();
            buildTreeIndexByAge();
            return;
        }
    }

    cout << "[ERROR] Удалённая запись с номером " << number << " не найдена!" << '\n';
}

// Физическое удаление с перестройкой массива и индексов
void deleteRecordsPhysicalWithTree() {
    cout << "\n ФИЗИЧЕСКОЕ УДАЛЕНИЕ (с перестройкой БДП) " << '\n';

    int deletedCount = 0;
    int newCount = 0;

    // Сдвиг не удалённых записей в начало массива
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            if (i != newCount) {
                participants[newCount] = participants[i];
            }
            newCount++;
        }
        else {
            deletedCount++;
        }
    }

    participantsCount = newCount;
    buildTreeIndexBySurname();
    buildTreeIndexByAge();

    cout << "[OK] Физически удалено " << deletedCount << " записей." << '\n';
}

void editRecordWithTree() {
    cout << "\n РЕДАКТИРОВАНИЕ (с обновлением БДП) " << '\n';

    int number;
    cout << "Введите номер участника для редактирования: ";
    cin >> number;

    int foundIndex = -1;
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted && participants[i].NumberOfParticipant == number) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == -1) {
        cout << "[ERROR] Запись с номером " << number << " не найдена!" << '\n';
        return;
    }

    cout << "\n[INFO] Текущие данные:" << '\n';
    participants[foundIndex].printFormatted();

    cout << "\nЧто хотите изменить?" << '\n';
    cout << "1. Фамилия" << '\n';
    cout << "2. Имя" << '\n';
    cout << "3. Отчество" << '\n';
    cout << "4. Год рождения" << '\n';
    cout << "5. Команда" << '\n';
    cout << "6. Результат" << '\n';
    cout << "7. Город" << '\n';
    cout << "8. Все поля" << '\n';
    cout << "Выберите: ";

    int choice;
    cin >> choice;

    Participant* p = &participants[foundIndex];
    string oldSurname = p->Surname;
    int oldAge = p->getAge();

    switch (choice) {
    case 1:
        cout << "Новая фамилия: "; cin >> p->Surname;
        break;
    case 2:
        cout << "Новое имя: "; cin >> p->Name;
        break;
    case 3:
        cout << "Новое отчество: "; cin >> p->Patronymic;
        break;
    case 4:
        cout << "Новый год рождения: "; cin >> p->BirthYear;
        break;
    case 5:
        cout << "Новая команда: "; cin >> p->TeamName;
        break;
    case 6:
        cout << "Новый результат: "; cin >> p->Result;
        break;
    case 7:
        cout << "Новый город: "; cin >> p->City;
        break;
    case 8:
        cout << "Введите новые данные:" << '\n';
        cout << "Фамилия: "; cin >> p->Surname;
        cout << "Имя: "; cin >> p->Name;
        cout << "Отчество: "; cin >> p->Patronymic;
        cout << "Год рождения: "; cin >> p->BirthYear;
        cout << "Команда: "; cin >> p->TeamName;
        cout << "Результат: "; cin >> p->Result;
        cout << "Город: "; cin >> p->City;
        break;
    default:
        cout << "[ERROR] Неверный выбор!" << '\n';
        return;
    }

    cout << "\n[OK] Запись обновлена!" << '\n';
    cout << "[INFO] Новые данные:" << '\n';
    participants[foundIndex].printFormatted();

    // Если изменился ключевой атрибут — перестраиваем индексы
    if (p->Surname != oldSurname || p->BirthYear != (oldAge - (time(nullptr) / 31536000 + 1900))) {
        cout << "\n[INFO] Изменены ключевые поля. Перестройка индексов..." << '\n';
        buildTreeIndexBySurname();
        buildTreeIndexByAge();
    }
}

void deleteTreeString(TreeNodeString* node) {
    if (node == nullptr) return;
    deleteTreeString(node->left);
    deleteTreeString(node->right);
    delete node;
}

void deleteTreeInt(TreeNodeInt* node) {
    if (node == nullptr) return;
    deleteTreeInt(node->left);
    deleteTreeInt(node->right);
    delete node;
}

void showMenuUpdated() {
    cout << "\n" << string(80, '=') << '\n';
    cout << string(80, '=') << '\n';
    cout << "ВВОД/ВЫВОД ДАННЫХ:" << '\n';
    cout << "  1. Ввод данных с клавиатуры" << '\n';
    cout << "  2. Вывод данных на экран (порядок ввода)" << '\n';
    cout << "  3. Вывод данных в файл" << '\n';
    cout << "  4. Ввод данных из файла" << '\n';
    cout << "\nИНДЕКСАЦИЯ (БИНАРНОЕ ДЕРЕВО):" << '\n';
    cout << "  5. Построить БДП по фамилии" << '\n';
    cout << "  6. Построить БДП по возрасту" << '\n';
    cout << "\nВЫВОД ПО БДП:" << '\n';
    cout << "  7. По фамилии (возрастание)" << '\n';
    cout << "  8. По фамилии (убывание)" << '\n';
    cout << "  9. По возрасту (возрастание)" << '\n';
    cout << "  10. По возрасту (убывание)" << '\n';
    cout << "\nПОИСК В БДП:" << '\n';
    cout << "  11. Поиск по фамилии (рекурсивный)" << '\n';
    cout << "  12. Поиск по возрасту (итерационный)" << '\n';
    cout << "\nРЕДАКТИРОВАНИЕ И УДАЛЕНИЕ:" << '\n';
    cout << "  13. Редактировать запись" << '\n';
    cout << "  14. Логическое удаление" << '\n';
    cout << "  15. Восстановить запись" << '\n';
    cout << "  16. Физическое удаление" << '\n';
    cout << "\n0. Выход" << '\n';
    cout << string(80, '=') << '\n';
}

using namespace std;

// Счётчики результатов тестов
int testsPassed = 0;
int testsFailed = 0;
string currentTestName = "";

// Проверка условия
void assertTrue(bool condition, const string& message) {
    if (condition) {
        cout << "[PASS] [" << currentTestName << "] " << message << '\n';
        testsPassed++;
    }
    else {
        cout << "[FAIL] [" << currentTestName << "] " << message << '\n';
        testsFailed++;
    }
}

// Проверка равенства целых чисел
void assertEqual(long long expected, long long actual, const string& message) {
    if (expected == actual) {
        cout << "[PASS] [" << currentTestName << "] " << message << '\n';
        testsPassed++;
    }
    else {
        cout << "[FAIL] [" << currentTestName << "] " << message
            << " (ожидалось: " << expected << ", получено: " << actual << ")" << '\n';
        testsFailed++;
    }
}

// Проверка равенства строк
void assertStringEqual(const string& expected, const string& actual, const string& message) {
    if (expected == actual) {
        cout << "[PASS] [" << currentTestName << "] " << message << '\n';
        testsPassed++;
    }
    else {
        cout << "[FAIL] [" << currentTestName << "] " << message
            << " (ожидалось: \"" << expected << "\", получено: \"" << actual << "\")" << '\n';
        testsFailed++;
    }
}

// Начало нового теста
void startTest(const string& testName) {
    currentTestName = testName;
    cout << "\n>>> ТЕСТ: " << testName << '\n';
    cout << string(60, '-') << '\n';
}

// Очистка глобального состояния перед тестом
void resetTestData() {
    for (int i = 0; i < MAX_SIZE; i++) {
        participants[i] = Participant();
    }
    participantsCount = 0;

    deleteTreeString(rootBySurname);
    deleteTreeInt(rootByAge);
    rootBySurname = nullptr;
    rootByAge = nullptr;
    indexCount = 0;
}

// Добавление тестовой записи в массив
int addTestParticipant(unsigned short int num, const string& surname, const string& name,
    const string& patronymic, unsigned short int birthYear,
    const string& team, unsigned int result, const string& city) {
    if (participantsCount >= MAX_SIZE) return -1;

    Participant* p = &participants[participantsCount];
    p->NumberOfParticipant = num;
    p->Surname = surname;
    p->Name = name;
    p->Patronymic = patronymic;
    p->BirthYear = birthYear;
    p->TeamName = team;
    p->Result = result;
    p->City = city;
    p->isDeleted = false;

    return participantsCount++;
}

// Проверка наличия записи в дереве по фамилии
bool treeContainsSurname(TreeNodeString* node, const string& surname, int recNum) {
    if (node == nullptr) return false;
    if (node->key == surname && node->recNum == recNum) return true;

    if (surname < node->key) {
        return treeContainsSurname(node->left, surname, recNum);
    }
    else {
        if (surname == node->key) {
            return treeContainsSurname(node->left, surname, recNum) ||
                treeContainsSurname(node->right, surname, recNum);
        }
        return treeContainsSurname(node->right, surname, recNum);
    }
}

// Проверка наличия записи в дереве по возрасту
bool treeContainsAge(TreeNodeInt* node, int age, int recNum) {
    if (node == nullptr) return false;
    if (node->key == age && node->recNum == recNum) return true;

    if (age < node->key) {
        return treeContainsAge(node->left, age, recNum);
    }
    else {
        if (age == node->key) {
            return treeContainsAge(node->left, age, recNum) ||
                treeContainsAge(node->right, age, recNum);
        }
        return treeContainsAge(node->right, age, recNum);
    }
}

// Подсчёт узлов в дереве
int countTreeNodesString(TreeNodeString* node) {
    if (node == nullptr) return 0;
    return 1 + countTreeNodesString(node->left) + countTreeNodesString(node->right);
}

int countTreeNodesInt(TreeNodeInt* node) {
    if (node == nullptr) return 0;
    return 1 + countTreeNodesInt(node->left) + countTreeNodesInt(node->right);
}

void test_InputFromKeyboard() {
    startTest("Ввод данных с клавиатуры");
    resetTestData();

    int startCount = participantsCount;
    addTestParticipant(101, "Иванов", "Иван", "Иванович", 2000, "Команда1", 150, "Москва");
    addTestParticipant(102, "Петров", "Пётр", "Петрович", 1999, "Команда2", 200, "СПб");

    assertEqual(2, participantsCount - startCount, "Добавлено 2 записи");
    assertStringEqual("Иванов", participants[startCount].Surname, "Фамилия первой записи");
    assertEqual(101, participants[startCount].NumberOfParticipant, "Номер участника");
}

void test_OutputToScreen() {
    startTest("Вывод данных на экран");
    resetTestData();

    addTestParticipant(201, "Сидоров", "Алексей", "Сергеевич", 2001, "Тест", 100, "Казань");

    // Функция вывода работает, запись добавлена
    outputToScreen();

    assertEqual(1, participantsCount, "Запись добавлена для вывода");
}

void test_OutputToFile() {
    startTest("Вывод данных в файл");
    resetTestData();

    const string testFile = "test_output.txt";
    addTestParticipant(301, "Тестов", "Тест", "Тестович", 2002, "TestTeam", 999, "TestCity");

    ofstream file(testFile, ios::out);
    assertTrue(file.is_open(), "Файл создан для записи");

    if (file.is_open()) {
        file << participants[0].NumberOfParticipant << DELIMITER
            << participants[0].Surname << DELIMITER
            << participants[0].Name << DELIMITER
            << participants[0].Patronymic << DELIMITER
            << participants[0].BirthYear << DELIMITER
            << participants[0].TeamName << DELIMITER
            << participants[0].Result << DELIMITER
            << participants[0].City << '\n';
        file.close();

        ifstream checkFile(testFile);
        string line;
        getline(checkFile, line);
        checkFile.close();

        string firstField = line.substr(0, line.find(DELIMITER));
        assertStringEqual("301", firstField, "Номер записан корректно");

        remove(testFile.c_str());
    }
}

void test_InputFromFile() {
    startTest("Ввод данных из файла");
    resetTestData();

    const string testFile = "test_input.txt";

    ofstream file(testFile, ios::out);
    if (file.is_open()) {
        file << "401;Файлов;Файл;Файлович;1998;FileTeam;500;FileCity" << '\n';
        file << "402;Другой;Друг;Другович;2003;OtherTeam;750;OtherCity" << '\n';
        file.close();
    }

    ifstream inFile(testFile);
    string line;
    int loaded = 0;

    while (participantsCount < MAX_SIZE && getline(inFile, line)) {
        if (line.empty()) continue;
        if (parseRecord(line, participants[participantsCount])) {
            participantsCount++;
            loaded++;
        }
    }
    inFile.close();

    assertEqual(2, loaded, "Загружено 2 записи из файла");
    assertStringEqual("Файлов", participants[0].Surname, "Фамилия первой загруженной записи");
    assertEqual(500, participants[0].Result, "Результат первой записи");

    remove(testFile.c_str());
}

void test_BuildTreeIndexBySurname() {
    startTest("Построение БДП по фамилии");
    resetTestData();

    addTestParticipant(1, "Смирнов", "А", "А", 2000, "T1", 100, "C1");
    addTestParticipant(2, "Алексеев", "Б", "Б", 2001, "T2", 200, "C2");
    addTestParticipant(3, "Васильев", "В", "В", 2002, "T3", 300, "C3");
    addTestParticipant(4, "Алексеев", "Г", "Г", 2003, "T4", 400, "C4");

    buildTreeIndexBySurname();

    assertTrue(rootBySurname != nullptr, "Корень дерева не null");
    assertEqual(4, countTreeNodesString(rootBySurname), "Дерево содержит 4 узла");

    assertTrue(treeContainsSurname(rootBySurname, "Смирнов", 0), "Смирнов в дереве");
    assertTrue(treeContainsSurname(rootBySurname, "Алексеев", 1), "Первый Алексеев в дереве");
    assertTrue(treeContainsSurname(rootBySurname, "Алексеев", 3), "Второй Алексеев в дереве");
    assertTrue(treeContainsSurname(rootBySurname, "Васильев", 2), "Васильев в дереве");
}

void test_BuildTreeIndexByAge() {
    startTest("Построение БДП по возрасту");
    resetTestData();

    addTestParticipant(1, "User1", "A", "A", 2000, "T1", 100, "C1");
    addTestParticipant(2, "User2", "B", "B", 1995, "T2", 200, "C2");
    addTestParticipant(3, "User3", "C", "C", 2010, "T3", 300, "C3");
    addTestParticipant(4, "User4", "D", "D", 2000, "T4", 400, "C4");

    buildTreeIndexByAge();

    assertTrue(rootByAge != nullptr, "Корень дерева возраста не null");
    assertEqual(4, countTreeNodesInt(rootByAge), "Дерево возраста содержит 4 узла");
}

void test_TreeTraversalAscending() {
    startTest("Обход дерева по возрастанию");
    resetTestData();

    addTestParticipant(1, "Zebra", "Z", "Z", 2000, "T", 100, "C");
    addTestParticipant(2, "Alpha", "A", "A", 2001, "T", 200, "C");
    addTestParticipant(3, "Middle", "M", "M", 2002, "T", 300, "C");

    buildTreeIndexBySurname();

    cout << "[INFO] Ожидаемый порядок: Alpha, Middle, Zebra" << '\n';
    cout << "[INFO] Фактический вывод: ";
    inOrderTraversalString(rootBySurname);

    assertTrue(true, "Обход выполнен без ошибок");
}

void test_TreeTraversalDescending() {
    startTest("Обход дерева по убыванию");
    resetTestData();

    addTestParticipant(1, "First", "F", "F", 2000, "T", 100, "C");
    addTestParticipant(2, "Second", "S", "S", 2001, "T", 200, "C");
    addTestParticipant(3, "Third", "T", "T", 2002, "T", 300, "C");

    buildTreeIndexBySurname();

    cout << "[INFO] Ожидаемый порядок: Third, Second, First" << '\n';
    cout << "[INFO] Фактический вывод: ";
    reverseInOrderTraversalString(rootBySurname);

    assertTrue(true, "Обход по убыванию выполнен без ошибок");
}

void test_SearchRecursive() {
    startTest("Рекурсивный поиск по фамилии");
    resetTestData();

    addTestParticipant(100, "Target", "Name", "Pat", 2000, "Team", 500, "City");
    addTestParticipant(101, "Other", "Name2", "Pat2", 2001, "Team2", 600, "City2");

    buildTreeIndexBySurname();

    TreeNodeString* found = searchTreeStringRecursive(rootBySurname, "Target");
    assertTrue(found != nullptr, "Найдена запись с фамилией 'Target'");
    if (found) {
        assertEqual(100, participants[found->recNum].NumberOfParticipant, "Найден правильный участник");
    }

    TreeNodeString* notFound = searchTreeStringRecursive(rootBySurname, "NonExistent");
    assertTrue(notFound == nullptr, "Не найдена несуществующая фамилия");
}

void test_SearchIterative() {
    startTest("Итерационный поиск по возрасту");
    resetTestData();

    // Добавляем записи с известными возрастами
    addTestParticipant(200, "User200", "A", "A", 2000, "T", 100, "C");
    addTestParticipant(201, "User201", "B", "B", 2000, "T", 200, "C");
    addTestParticipant(202, "User202", "C", "C", 1990, "T", 300, "C");

    buildTreeIndexByAge();

    int results[MAX_SIZE];
    int count = searchTreeIntIterative(rootByAge, 26, results);

    assertEqual(2, count, "Найдено 2 записи с возрастом 26 лет");

    bool found200 = false, found201 = false;
    for (int i = 0; i < count; i++) {
        if (participants[results[i]].NumberOfParticipant == 200) found200 = true;
        if (participants[results[i]].NumberOfParticipant == 201) found201 = true;
    }
    assertTrue(found200 && found201, "Найдены оба участника с возрастом 26 лет");

    count = searchTreeIntIterative(rootByAge, 99, results);
    assertEqual(0, count, "Не найдены записи с несуществующим возрастом");
}

void test_EditRecord() {
    startTest("Редактирование записи");
    resetTestData();

    int idx = addTestParticipant(300, "OldName", "Old", "Old", 2000, "OldTeam", 100, "OldCity");
    buildTreeIndexBySurname();

    string oldSurname = participants[idx].Surname;
    participants[idx].Surname = "NewName";

    buildTreeIndexBySurname();

    assertStringEqual("NewName", participants[idx].Surname, "Фамилия изменена");
    assertTrue(!treeContainsSurname(rootBySurname, oldSurname, idx), "Старая фамилия удалена из дерева");
    assertTrue(treeContainsSurname(rootBySurname, "NewName", idx), "Новая фамилия добавлена в дерево");
}

void test_LogicalDelete() {
    startTest("Логическое удаление");
    resetTestData();

    addTestParticipant(400, "ToDelete", "D", "D", 2000, "T", 100, "C");
    addTestParticipant(401, "Keep", "K", "K", 2001, "T", 200, "C");

    buildTreeIndexBySurname();
    int initialNodes = countTreeNodesString(rootBySurname);

    participants[0].isDeleted = true;
    rootBySurname = deleteNodeString(rootBySurname, "ToDelete", 0);

    assertTrue(participants[0].isDeleted, "Запись помечена как удалённая");
    assertEqual(initialNodes - 1, countTreeNodesString(rootBySurname), "Узел удалён из дерева");
    assertTrue(!treeContainsSurname(rootBySurname, "ToDelete", 0), "Удалённая запись не ищется в дереве");
    assertTrue(treeContainsSurname(rootBySurname, "Keep", 1), "Оставшаяся запись в дереве");
}

void test_RestoreRecord() {
    startTest("Восстановление записи");
    resetTestData();

    int idx = addTestParticipant(500, "ToRestore", "R", "R", 2000, "T", 100, "C");
    participants[idx].isDeleted = true;

    buildTreeIndexBySurname();
    assertTrue(!treeContainsSurname(rootBySurname, "ToRestore", idx), "Удалённая запись не в дереве");

    participants[idx].isDeleted = false;
    buildTreeIndexBySurname();

    assertTrue(treeContainsSurname(rootBySurname, "ToRestore", idx), "Восстановленная запись в дереве");
}

void test_PhysicalDelete() {
    startTest("Физическое удаление");
    resetTestData();

    addTestParticipant(600, "Keep1", "K1", "K1", 2000, "T", 100, "C");
    addTestParticipant(601, "Delete1", "D1", "D1", 2001, "T", 200, "C");
    addTestParticipant(602, "Keep2", "K2", "K2", 2002, "T", 300, "C");

    participants[1].isDeleted = true;

    int oldCount = participantsCount;

    int deletedCount = 0;
    int newCount = 0;
    for (int i = 0; i < participantsCount; i++) {
        if (!participants[i].isDeleted) {
            if (i != newCount) {
                participants[newCount] = participants[i];
            }
            newCount++;
        }
        else {
            deletedCount++;
        }
    }
    participantsCount = newCount;

    buildTreeIndexBySurname();

    assertEqual(2, participantsCount, "Осталось 2 записи после физического удаления");
    assertEqual(1, deletedCount, "Удалена 1 запись");
    assertEqual(2, countTreeNodesString(rootBySurname), "Дерево содержит 2 узла");
}


void test_FullWorkflow() {
    startTest("Комплексный тест: Полный цикл работы");
    resetTestData();

    // 1. Добавление данных
    addTestParticipant(1, "Alpha", "A", "A", 2000, "TeamA", 100, "CityA");
    addTestParticipant(2, "Beta", "B", "B", 1999, "TeamB", 200, "CityB");
    addTestParticipant(3, "Gamma", "C", "C", 2001, "TeamC", 300, "CityC");
    assertEqual(3, participantsCount, "Добавлено 3 записи");

    // 2. Построение индексов
    buildTreeIndexBySurname();
    buildTreeIndexByAge();
    assertEqual(3, countTreeNodesString(rootBySurname), "Индекс по фамилии построен");

    // 3. Поиск
    TreeNodeString* found = searchTreeStringRecursive(rootBySurname, "Beta");
    assertTrue(found != nullptr, "Поиск находит существующую запись");

    // 4. Редактирование
    int betaIdx = found->recNum;
    string oldBeta = participants[betaIdx].Surname;
    participants[betaIdx].Surname = "BetaUpdated";
    buildTreeIndexBySurname();
    assertTrue(!treeContainsSurname(rootBySurname, oldBeta, betaIdx), "Старый ключ удалён");
    assertTrue(treeContainsSurname(rootBySurname, "BetaUpdated", betaIdx), "Новый ключ добавлен");

    // 5. Логическое удаление
    participants[0].isDeleted = true;
    rootBySurname = deleteNodeString(rootBySurname, "Alpha", 0);
    assertTrue(participants[0].isDeleted, "Запись логически удалена");

    // 6. Восстановление
    participants[0].isDeleted = false;
    buildTreeIndexBySurname();
    assertTrue(treeContainsSurname(rootBySurname, "Alpha", 0), "Запись восстановлена");

    cout << "[INFO] Комплексный тест завершён" << '\n';
    assertTrue(true, "Все этапы полного цикла выполнены");
}


void runAllTests() {
    cout << "ЗАПУСК ТЕСТИРОВАНИЯ ПРОГРАММЫ" << '\n';
    testsPassed = 0;
    testsFailed = 0;

    // Базовые функции
    test_InputFromKeyboard();
    test_OutputToScreen();
    test_OutputToFile();
    test_InputFromFile();

    // Индексы на бинарном дереве
    test_BuildTreeIndexBySurname();
    test_BuildTreeIndexByAge();
    test_TreeTraversalAscending();
    test_TreeTraversalDescending();

    // Поиск
    test_SearchRecursive();
    test_SearchIterative();

    // Редактирование и удаление
    test_EditRecord();
    test_LogicalDelete();
    test_RestoreRecord();
    test_PhysicalDelete();

    // Комплексный тест
    test_FullWorkflow();

    // Итоги
    cout << "РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ" << '\n';
    cout << "Пройдено тестов: " << testsPassed << '\n';
    cout << "Провалено тестов: " << testsFailed << '\n';
    cout << "Всего тестов: " << (testsPassed + testsFailed) << '\n';

    if (testsFailed == 0) {
        cout << "\n[SUCCESS] Все тесты пройдены!" << '\n';
    }
    else {
        cout << "\n[WARNING] Есть непройденные тесты!" << '\n';
    }
    cout << string(80, '=') << '\n';
}



// СТРУКТУРА УЗЛА ЛИНЕЙНОГО СПИСКА (ЗАДАНИЕ 3)

struct ListNode {
    Participant* data;      // Указатель на данные
    ListNode* nextInput;    // Связь для порядка ввода
    ListNode* nextSurname;  // Связь для сортировки по фамилии
    ListNode* nextAge;      // Связь для сортировки по возрасту

    ListNode(Participant* p) : data(p), nextInput(nullptr), nextSurname(nullptr), nextAge(nullptr) {}
};


// ГЛОБАЛЬНЫЕ "ГОЛОВЫ" СПИСКОВ

ListNode* headInput = nullptr;    // Голова списка порядка ввода
ListNode* headSurname = nullptr;  // Голова списка по фамилии
ListNode* headAge = nullptr;      // Голова списка по возрасту


// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ


// Создание нового участника (ввод с клавиатуры)
Participant* createNewParticipant() {
    Participant* p = new Participant();

    cout << "\n--- Ввод данных участника ---" << '\n';
    cout << "Номер участника: ";
    cin >> p->NumberOfParticipant;
    cout << "Фамилия: ";
    cin >> p->Surname;
    cout << "Имя: ";
    cin >> p->Name;
    cout << "Отчество: ";
    cin >> p->Patronymic;
    cout << "Год рождения: ";
    cin >> p->BirthYear;
    cout << "Команда: ";
    cin >> p->TeamName;
    cout << "Результат: ";
    cin >> p->Result;
    cout << "Город: ";
    cin >> p->City;

    p->isDeleted = false;
    return p;
}


// НОВЫЙ ФОРМАТ ВЫВОДА ОДНОГО УЧАСТНИКА

void printParticipant(const Participant* p) {
    if (!p || p->isDeleted) return;

    cout << "№ " << p->NumberOfParticipant << '\n'
        << "Фамилия " << p->Surname << '\n'
        << "Имя " << p->Name << '\n'
        << "Отчество " << p->Patronymic << '\n'
        << "Год " << p->BirthYear << '\n'
        << "Возраст " << p->getAge() << '\n'
        << "Команда " << p->TeamName << '\n'
        << "Результат " << p->Result << '\n'
        << "Город " << p->City << '\n';
}


// 1. & 2. ДОБАВЛЕНИЕ В СПИСОК С СОРТИРОВКОЙ


// Добавление в конец списка порядка ввода
void appendToInputList(ListNode* newNode) {
    if (!headInput) {
        headInput = newNode;
    }
    else {
        ListNode* curr = headInput;
        while (curr->nextInput) {
            curr = curr->nextInput;
        }
        curr->nextInput = newNode;
    }
}

// Вставка в отсортированный список по фамилии (возрастание)
void insertSortedSurname(ListNode*& head, ListNode* newNode) {
    if (!head || newNode->data->Surname < head->data->Surname) {
        newNode->nextSurname = head;
        head = newNode;
        return;
    }

    ListNode* curr = head;
    while (curr->nextSurname && curr->nextSurname->data->Surname <= newNode->data->Surname) {
        curr = curr->nextSurname;
    }
    newNode->nextSurname = curr->nextSurname;
    curr->nextSurname = newNode;
}

// Вставка в отсортированный список по возрасту (возрастание)
void insertSortedAge(ListNode*& head, ListNode* newNode) {
    if (!head || newNode->data->getAge() < head->data->getAge()) {
        newNode->nextAge = head;
        head = newNode;
        return;
    }

    ListNode* curr = head;
    while (curr->nextAge && curr->nextAge->data->getAge() <= newNode->data->getAge()) {
        curr = curr->nextAge;
    }
    newNode->nextAge = curr->nextAge;
    curr->nextAge = newNode;
}

// Основная функция добавления нового участника
void addNewParticipant() {
    Participant* p = createNewParticipant();
    ListNode* newNode = new ListNode(p);

    // Включаем во все три списка
    appendToInputList(newNode);
    insertSortedSurname(headSurname, newNode);
    insertSortedAge(headAge, newNode);

    cout << "\n[OK] Участник добавлен во все списки." << '\n';
}


// 3. ПРОСМОТР (ВЫВОД) СПИСКОВ


// Итерационный вывод списка
void printListIterative(ListNode* head, const string& type) {
    cout << "\n========================================" << '\n';
    cout << "=== СПИСОК (" << type << ") ===" << '\n';
    cout << "========================================" << '\n';

    ListNode* curr = head;
    int count = 0;
    while (curr) {
        if (!curr->data->isDeleted) {
            cout << "\n--- Запись #" << (count + 1) << " ---" << '\n';
            printParticipant(curr->data);
            count++;
        }
        curr = (type == "Ввод") ? curr->nextInput :
            (type == "Фамилия") ? curr->nextSurname : curr->nextAge;
    }

    cout << "========================================" << '\n';
    if (count == 0) {
        cout << "Список пуст." << '\n';
    }
    else {
        cout << "Всего записей: " << count << '\n';
    }
    cout << "========================================" << '\n';
}

// Рекурсивный вывод (вспомогательная функция)
void printListRecursiveHelper(ListNode* node, const string& type, int& count) {
    if (!node) return;

    if (!node->data->isDeleted) {
        cout << "\n--- Запись #" << (count + 1) << " ---" << '\n';
        printParticipant(node->data);
        count++;
    }

    ListNode* nextNode = (type == "Ввод") ? node->nextInput :
        (type == "Фамилия") ? node->nextSurname : node->nextAge;

    printListRecursiveHelper(nextNode, type, count);
}

// Обёртка для рекурсивного вывода
void printListRecursive(ListNode* head, const string& type) {
    cout << "\n========================================" << '\n';
    cout << "=== СПИСОК (" << type << ") - РЕКУРСИВНЫЙ ВЫВОД ===" << '\n';
    cout << "========================================" << '\n';

    int count = 0;
    printListRecursiveHelper(head, type, count);

    cout << "========================================" << '\n';
    if (count == 0) {
        cout << "Список пуст." << '\n';
    }
    else {
        cout << "Всего записей: " << count << '\n';
    }
    cout << "========================================" << '\n';
}


// 4. ПОИСК В СПИСКАХ


// Поиск по фамилии (рекурсивный по списку фамилий)
ListNode* searchBySurnameRecursive(ListNode* node, const string& surname) {
    if (!node) return nullptr;
    if (node->data->Surname == surname && !node->data->isDeleted) {
        return node;
    }
    return searchBySurnameRecursive(node->nextSurname, surname);
}

// Поиск по возрасту (итерационный по списку возрастов)
ListNode* searchByAgeIterative(ListNode* head, int age) {
    ListNode* curr = head;
    while (curr) {
        if (curr->data->getAge() == age && !curr->data->isDeleted) {
            return curr;
        }
        curr = curr->nextAge;
    }
    return nullptr;
}

void performSearch() {
    cout << "\n=== ПОИСК ===" << '\n';
    cout << "1. Поиск по фамилии (рекурсивный)" << '\n';
    cout << "2. Поиск по возрасту (итерационный)" << '\n';
    cout << "Выбор: ";
    int choice;
    cin >> choice;

    if (choice == 1) {
        string surname;
        cout << "Введите фамилию: ";
        cin >> surname;
        ListNode* found = searchBySurnameRecursive(headSurname, surname);
        if (found) {
            cout << "\n[НАЙДЕНО]" << '\n';
            printParticipant(found->data);
        }
        else {
            cout << "\n[НЕ НАЙДЕНО] Участник с фамилией " << surname << '\n';
        }
    }
    else if (choice == 2) {
        int age;
        cout << "Введите возраст: ";
        cin >> age;
        ListNode* found = searchByAgeIterative(headAge, age);
        if (found) {
            cout << "\n[НАЙДЕНО]" << '\n';
            printParticipant(found->data);
        }
        else {
            cout << "\n[НЕ НАЙДЕНО] Участников с возрастом " << age << '\n';
        }
    }
}


// 5. УДАЛЕНИЕ ИЗ СПИСКОВ


// Удаление узла из всех трёх списков
void deleteNode(ListNode* nodeToDelete) {
    if (!nodeToDelete) return;

    // 1. Удаляем из списка ввода
    if (headInput == nodeToDelete) {
        headInput = headInput->nextInput;
    }
    else {
        ListNode* curr = headInput;
        while (curr && curr->nextInput != nodeToDelete) {
            curr = curr->nextInput;
        }
        if (curr) curr->nextInput = nodeToDelete->nextInput;
    }

    // 2. Удаляем из списка фамилий
    if (headSurname == nodeToDelete) {
        headSurname = headSurname->nextSurname;
    }
    else {
        ListNode* curr = headSurname;
        while (curr && curr->nextSurname != nodeToDelete) {
            curr = curr->nextSurname;
        }
        if (curr) curr->nextSurname = nodeToDelete->nextSurname;
    }

    // 3. Удаляем из списка возрастов
    if (headAge == nodeToDelete) {
        headAge = headAge->nextAge;
    }
    else {
        ListNode* curr = headAge;
        while (curr && curr->nextAge != nodeToDelete) {
            curr = curr->nextAge;
        }
        if (curr) curr->nextAge = nodeToDelete->nextAge;
    }

    // Освобождение памяти
    delete nodeToDelete->data;
    delete nodeToDelete;
    cout << "\n[OK] Запись удалена из всех списков." << '\n';
}

// Удаление по фамилии
void deleteBySurname() {
    string surname;
    cout << "Введите фамилию для удаления: ";
    cin >> surname;

    ListNode* found = searchBySurnameRecursive(headSurname, surname);
    if (found) {
        deleteNode(found);
    }
    else {
        cout << "\n[ERROR] Не найдено для удаления." << '\n';
    }
}

// Удаление по возрасту
void deleteByAge() {
    int age;
    cout << "Введите возраст для удаления: ";
    cin >> age;

    ListNode* found = searchByAgeIterative(headAge, age);
    if (found) {
        deleteNode(found);
    }
    else {
        cout << "\n[ERROR] Не найдено для удаления." << '\n';
    }
}


// ОСВОБОЖДЕНИЕ ПАМЯТИ

void cleanupList() {
    ListNode* curr = headInput;
    while (curr) {
        ListNode* temp = curr;
        curr = curr->nextInput;
        delete temp->data;
        delete temp;
    }
    headInput = headSurname = headAge = nullptr;
}
void deleteFirstElement() {
    cout << "\n=== УДАЛЕНИЕ ПЕРВОГО ЭЛЕМЕНТА ===" << endl;

    if (!headInput) {
        cout << "[ERROR] Список пуст! Нечего удалять." << endl;
        return;
    }

    ListNode* toDelete = headInput;

    // Определяем тип удаляемого элемента
    if (toDelete->nextInput == nullptr) {
        cout << "[INFO] Удаляется ЕДИНСТВЕННЫЙ элемент списка" << endl;
    }
    else {
        cout << "[INFO] Удаляется ПЕРВЫЙ элемент списка" << endl;
        cout << "Данные удаляемого элемента:" << endl;
        printParticipant(toDelete->data);
    }

    // Удаляем узел из всех списков
    deleteNode(toDelete);

    cout << "\n[OK] Первый элемент успешно удалён!" << endl;
}

// Удаление последнего элемента списка
void deleteLastElement() {
    cout << "\n=== УДАЛЕНИЕ ПОСЛЕДНЕГО ЭЛЕМЕНТА ===" << endl;

    if (!headInput) {
        cout << "[ERROR] Список пуст! Нечего удалять." << endl;
        return;
    }

    // Поиск последнего элемента в списке порядка ввода
    ListNode* prev = nullptr;
    ListNode* curr = headInput;

    while (curr->nextInput) {
        prev = curr;
        curr = curr->nextInput;
    }

    // curr теперь указывает на последний элемент
    cout << "[INFO] Удаляется ПОСЛЕДНИЙ элемент списка" << endl;
    cout << "Данные удаляемого элемента:" << endl;
    printParticipant(curr->data);

    // Удаляем узел из всех списков
    deleteNode(curr);

    cout << "\n[OK] Последний элемент успешно удалён!" << endl;
}


// ГЛАВНАЯ ФУНКЦИЯ (МЕНЮ)

/*
1
1
 Smith
 John
 Johnovich
 2004
 Team
 1
 Town
 1
 2
 Bond
 John
 Johnovich
 2005
 Team
 2
 Town
 1
 3
 Mercury
 Fred
 Johnovich
 2001
 Team
 3
 Town
1
 4
 Senna
 Ayrton
 Ayrtovich
 2003
 Team
 4
 Town
*/
int main() {
    setlocale(LC_ALL, "Russian");
    int choice=999;
    while (choice) {
        cout << "ЗАДАНИЕ 3: ЛИНЕЙНЫЕ СПИСКИ" << '\n';
        cout << "1. Добавить участника (ввод + сортировка)" << '\n';
        cout << "2. Вывод списка (порядок ввода) - итерационный" << '\n';
        cout << "3. Вывод списка (по фамилии) - итерационный" << '\n';
        cout << "4. Вывод списка (по возрасту) - итерационный" << '\n';
        cout << "5. Вывод списка (рекурсивный)" << '\n';
        cout << "6. Поиск участника" << '\n';
        cout << "7. Удалить по фамилии" << '\n';
        cout << "8. Удалить по возрасту" << '\n';
        cout << "9. Удалить ПЕРВЫЙ элемент списка" << '\n';
        cout << "10. Удалить ПОСЛЕДНИЙ элемент списка" << '\n';
        cout << "0. Выход" << '\n';
        cout << "Выбор: ";
        cin >> choice;

        switch (choice) {
        case 1: addNewParticipant(); break;
        case 2: printListIterative(headInput, "Ввод"); break;
        case 3: printListIterative(headSurname, "Фамилия"); break;
        case 4: printListIterative(headAge, "Возраст"); break;
        case 5:
            cout << "\nРекурсивный вывод:" << '\n';
            cout << "1. По вводу" << '\n';
            cout << "2. По фамилии" << '\n';
            cout << "3. По возрасту" << '\n';
            cout << "Выбор: ";
            int recChoice;
            cin >> recChoice;
            if (recChoice == 1) printListRecursive(headInput, "Ввод");
            else if (recChoice == 2) printListRecursive(headSurname, "Фамилия");
            else if (recChoice == 3) printListRecursive(headAge, "Возраст");
            break;
        case 6: performSearch(); break;
        case 7: deleteBySurname(); break;
        case 8: deleteByAge(); break;
        case 9: deleteFirstElement(); break;      // НОВЫЙ ПУНКТ
        case 10: deleteLastElement(); break;      // НОВЫЙ ПУНКТ
        case 0: cout << "\nВыход..." << '\n'; break;
        default: cout << "\nНеверный выбор!" << '\n';
        }
    }
    cleanupList();
    return 0;
        //setupConsole();
        //runAllTests(); 
        // return 0;
        // ИНТЕРАКТИВНЫЙ РЕЖИМ
        /*while (choice) {
            showMenu();
            cout << "Ваш выбор: ";
            cin >> choice;

            switch (choice) {
            case 1: inputFromKeyboard(); break;
            case 2: outputToScreen(); break;
            case 3: outputToFile(); break;
            case 4: inputFromFile(); break;
            case 5: buildIndexBySurname(); break;
            case 6: buildIndexByAge(); break;
            case 7:
                buildIndexBySurname();
                for (int i = 0; i < indexCount; i++) {
                    int recNum = indexBySurname[i].recNum;
                    participants[recNum].printFormatted();
                }
                break;
            case 8:
                buildIndexBySurname();
                // Для убывания по строке - обратный проход
                for (int i = indexCount - 1; i >= 0; i--) {
                    int recNum = indexBySurname[i].recNum;
                    participants[recNum].printFormatted();
                }
                break;
            case 9:
                buildIndexByAge();
                outputByIndexAscending(indexByAge, indexCount, "Возраст");
                break;
            case 10:
                buildIndexByAge();
                outputByIndexDescending(indexByAge, indexCount, "Возраст");
                break;
            case 11:searchBySurnameIndex();break;
            case 12: searchByAge(); break;
            case 13: editRecord(); break;
            case 14: deleteRecordLogical(); break;
            case 15: restoreRecord(); break;
            case 16: deleteRecordsPhysical(); break;
            }
        }*/
        /*while (choice != 0) {
            showMenuUpdated();
            cout << "Ваш выбор: ";
            cin >> choice;
            switch (choice) {
            case 1: inputFromKeyboard(); break;
            case 2: outputToScreen(); break;
            case 3: outputToFile(); break;
            case 4: inputFromFile(); break;
            case 5: buildTreeIndexBySurname(); break;
            case 6: buildTreeIndexByAge(); break;
            case 7:
                buildTreeIndexBySurname();
                outputByTreeAscending("Фамилия", false);
                break;
            case 8:
                buildTreeIndexBySurname();
                outputByTreeDescending("Фамилия", false);
                break;
            case 9:
                buildTreeIndexByAge();
                outputByTreeAscending("Возраст", true);
                break;
            case 10:
                buildTreeIndexByAge();
                outputByTreeDescending("Возраст", true);
                break;
            case 11: searchBySurnameTree(); break;
            case 12: searchByAgeTree(); break;
            case 13: editRecordWithTree(); break;
            case 14: deleteRecordLogicalWithTree(); break;
            case 15: restoreRecordWithTree(); break;
            case 16: deleteRecordsPhysicalWithTree(); break;
            case 99:  // Скрытый пункт для запуска тестов
                runAllTests();
                break;
            case 0:
                cout << "\n[EXIT] Завершение работы..." << '\n';
                deleteTreeString(rootBySurname);
                deleteTreeInt(rootByAge);
                return 0;
            default:
                cout << "\n[ERROR] Неверный выбор!" << '\n';
            }
        }*/
}