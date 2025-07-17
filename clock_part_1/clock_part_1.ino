#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BTN_NEXT 2  // Кнопка "Следующий"
#define BTN_PREV 3  // Кнопка "Предыдущий"
#define BTN_SET 4   // Кнопка "Установка"

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Инициализация LCD 16x2 по I2C адресу 0x27

// Структура для хранения даты
struct DateTime {
  int year = 2023;
  int month = 1;
  int day = 1;
} currentDate;

// Переменные состояния
int hours = 12, minutes = 0, seconds = 0;  // Текущее время
bool settingMode = false;                  // Режим настройки
int settingCursor = 0;                     // Текущая позиция в настройках

// Временные переменные для настройки
int tempHoursIndex, tempMinutesIndex;      // Время для настройки
int tempMonthIndex, tempDayIndex;          // Дата для настройки
int tempYear;                              // Год для настройки
int tempDayCount;                          // Временное хранение дней в месяце

// Пользовательские символы (сердечки и часы)
byte heartFull[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};
byte heartEmpty[8] = {0x00, 0x0A, 0x15, 0x11, 0x0A, 0x04, 0x00, 0x00};
byte clockSymbol[8] = {0x00, 0x0E, 0x15, 0x17, 0x11, 0x0E, 0x00, 0x00};

// Защита от дребезга
unsigned long lastDebounceTimeNext = 0;
unsigned long lastDebounceTimePrev = 0;
unsigned long lastDebounceTimeSet = 0;
const unsigned long debounceDelay = 50;  // Задержка антидребезга

void setup() {
  // Настройка кнопок с подтяжкой к питанию
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_SET, INPUT_PULLUP);
  
  // Инициализация LCD
  lcd.init();
  lcd.backlight();
  
  // Создание пользовательских символов
  lcd.createChar(0, heartFull);    // Символ 0 - заполненное сердце
  lcd.createChar(1, heartEmpty);   // Символ 1 - пустое сердце
  lcd.createChar(2, clockSymbol);  // Символ 2 - часы
  
  // Установка начальной даты
  setCurrentDate(2023, 1, 1);  // Инициализация даты
  
  // Начальное сообщение
  lcd.setCursor(0, 0);
  lcd.print("   My Clock    ");
  lcd.setCursor(0, 1);
  lcd.print("  Version 1.0 ");
  delay(2000);
  lcd.clear();
}

void loop() {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();

  // Обновление времени каждую секунду
  if (currentMillis - lastUpdate >= 1000 && !settingMode) {
    lastUpdate = currentMillis;
    updateTime();  // Обновление счетчиков времени
  }

  handleButtons();  // Обработка нажатий кнопок
  displayScreen();  // Обновление дисплея
}

// Основная функция отображения
void displayScreen() {
  if (settingMode) {
    displaySettingScreen();  // Экран настройки
  } else {
    displayClock();         // Основной экран часов
  }
}

// Отображение основного экрана часов
void displayClock() {
  // Верхняя строка: время
  lcd.setCursor(0, 0);
  lcd.write(byte(2));  // Символ часов
  lcd.print("   ");
  printTwoDigits(hours);    // Часы (с ведущим нулем)
  lcd.print(":");
  printTwoDigits(minutes);  // Минуты (с ведущим нулем)
  lcd.print(":");
  printTwoDigits(seconds);  // Секунды (с ведущим нулем)
  
  // Нижняя строка: дата
  lcd.setCursor(0, 1);
  lcd.write(byte(1));  // Символ пустого сердца
  lcd.print("   ");
  printTwoDigits(currentDate.day);     // День
  lcd.print("/");
  printTwoDigits(currentDate.month);   // Месяц
  lcd.print("/");
  lcd.print(currentDate.year % 100);   // Год (две последние цифры)
  lcd.print("   ");
  lcd.write(byte(0));  // Символ заполненного сердца
  
  // Мигающее сердце в правом верхнем углу
  lcd.setCursor(15, 0);
  if (seconds % 2 == 0) {
    lcd.write(byte(0));  // Заполненное сердце
  } else {
    lcd.write(byte(1));  // Пустое сердце
  }
}

// Экран настроек
void displaySettingScreen() {
  lcd.setCursor(0, 0);
  lcd.print("Set: ");
  
  // Отображение названия настраиваемого параметра
  switch(settingCursor) {
    case 0: lcd.print("Hours"); break;   // Часы
    case 1: lcd.print("Minutes"); break; // Минуты
    case 2: lcd.print("Day"); break;     // День
    case 3: lcd.print("Month"); break;   // Месяц
    case 4: lcd.print("Year"); break;    // Год
  }
  
  // Отображение текущего значения
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  switch(settingCursor) {
    case 0: lcd.print(tempHoursIndex); break;      // Часы
    case 1: lcd.print(tempMinutesIndex); break;    // Минуты
    case 2: lcd.print(tempDayIndex + 1); break;    // День
    case 3: lcd.print(tempMonthIndex + 1); break;  // Месяц
    case 4: lcd.print(tempYear); break;            // Год
  }
}

// Обработка кнопок с защитой от дребезга
void handleButtons() {
  unsigned long currentMillis = millis();
  
  // Обработка кнопки NEXT (увеличение значения)
  if (digitalRead(BTN_NEXT) == LOW) {
    if (currentMillis - lastDebounceTimeNext > debounceDelay) {
      lastDebounceTimeNext = currentMillis;
      if (settingMode) {
        adjustSetting(1);  // Увеличение значения
        displaySettingScreen();
      }
    }
  }
  
  // Обработка кнопки PREV (уменьшение значения)
  if (digitalRead(BTN_PREV) == LOW) {
    if (currentMillis - lastDebounceTimePrev > debounceDelay) {
      lastDebounceTimePrev = currentMillis;
      if (settingMode) {
        adjustSetting(-1);  // Уменьшение значения
        displaySettingScreen();
      }
    }
  }

  // Обработка кнопки SET (вход/подтверждение)
  if (digitalRead(BTN_SET) == LOW) {
    if (currentMillis - lastDebounceTimeSet > debounceDelay) {
      lastDebounceTimeSet = currentMillis;
      
      if (!settingMode) {
        enterSettingMode();  // Вход в режим настройки
      } else {
        settingCursor++;     // Переход к следующему параметру
        processSettingCursor();
      }
    }
  }
}

// Вход в режим настройки
void enterSettingMode() {
  settingMode = true;       // Активация флага настройки
  settingCursor = 0;        // Сброс позиции курсора
  
  // Инициализация временных переменных
  tempHoursIndex = hours;   // Текущие часы
  tempMinutesIndex = minutes;  // Текущие минуты
  tempMonthIndex = currentDate.month - 1;  // Текущий месяц (0-11)
  tempYear = currentDate.year;  // Текущий год
  tempDayCount = daysInMonth(tempMonthIndex + 1, tempYear);  // Дней в месяце
  tempDayIndex = currentDate.day - 1;  // Текущий день (0-30)
  
  // Корректировка дня если выходит за пределы
  if (tempDayIndex >= tempDayCount) {
    tempDayIndex = tempDayCount - 1;
  }
  
  lcd.clear();
  displaySettingScreen();
}

// Обработка позиции курсора в настройках
void processSettingCursor() {
  // Если прошли все параметры (0-4)
  if (settingCursor > 4) {
    applySettings();    // Применяем настройки
    settingMode = false;  // Выходим из режима настройки
    lcd.clear();
  } else {
    displaySettingScreen();  // Иначе продолжаем настройку
  }
}

// Применение настроек
void applySettings() {
  hours = tempHoursIndex;         // Установка часов
  minutes = tempMinutesIndex;     // Установка минут
  seconds = 0;                    // Сброс секунд
  currentDate.day = tempDayIndex + 1;     // Установка дня
  currentDate.month = tempMonthIndex + 1; // Установка месяца
  currentDate.year = tempYear;            // Установка года
}

// Изменение значения параметра
void adjustSetting(int direction) {
  switch(settingCursor) {
    case 0:  // Часы
      tempHoursIndex = (tempHoursIndex + direction + 24) % 24;
      break;
    case 1:  // Минуты
      tempMinutesIndex = (tempMinutesIndex + direction + 60) % 60;
      break;
    case 2:  // День
      tempDayIndex = (tempDayIndex + direction + tempDayCount) % tempDayCount;
      break;
    case 3:  // Месяц
      tempMonthIndex = (tempMonthIndex + direction + 12) % 12;
      // Обновление количества дней при смене месяца
      tempDayCount = daysInMonth(tempMonthIndex + 1, tempYear);
      tempDayIndex = min(tempDayIndex, tempDayCount - 1);
      break;
    case 4:  // Год
      tempYear = constrain(tempYear + direction, 2000, 2099);
      // Обновление количества дней при смене года
      tempDayCount = daysInMonth(tempMonthIndex + 1, tempYear);
      tempDayIndex = min(tempDayIndex, tempDayCount - 1);
      break;
  }
}

// Установка текущей даты
void setCurrentDate(int y, int m, int d) {
  currentDate.year = y;
  currentDate.month = m;
  currentDate.day = d;
}

// Расчет дней в месяце
int daysInMonth(int month, int year) {
  if (month == 4 || month == 6 || month == 9 || month == 11) {
    return 30;  // Апрель, Июнь, Сентябрь, Ноябрь
  } else if (month == 2) {  // Февраль
    // Проверка високосного года
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
      return 29;
    } else {
      return 28;
    }
  } else {
    return 31;  // Остальные месяцы
  }
}

// Обновление времени
void updateTime() {
  if (++seconds >= 60) {
    seconds = 0;
    if (++minutes >= 60) {
      minutes = 0;
      if (++hours >= 24) {
        hours = 0;
        updateDate();  // Обновление даты при смене суток
      }
    }
  }
}

// Обновление даты
void updateDate() {
  currentDate.day++;
  int dim = daysInMonth(currentDate.month, currentDate.year);
  
  if (currentDate.day > dim) {
    currentDate.day = 1;
    if (++currentDate.month > 12) {
      currentDate.month = 1;
      currentDate.year++;
    }
  }
}

// Вывод чисел с ведущим нулем
void printTwoDigits(int number) {
  if (number < 10) lcd.print("0");  // Добавление ведущего нуля
  lcd.print(number);
}