using Microsoft.Win32;
using System.Security.Principal;

namespace RegistryLab
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.OutputEncoding = System.Text.Encoding.UTF8;

            if (!IsAdministrator())
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("УВАГА: Для доступу до HKLM та Планувальника завдань запустіть програму від імені АДМІНІСТРАТОРА.");
                Console.ResetColor();
                Console.WriteLine("Натисніть Enter для виходу...");
                Console.ReadLine();
                return;
            }

            try
            {
                // --- ЗАВДАННЯ 4: Список автозавантаження ---
                PrintHeader("4. Список програм автозавантаження");
                ListAutostartPrograms();

                // --- ЗАВДАННЯ 5: Додавання запису ---
                PrintHeader("5. Додавання програми в автозавантаження");
                string appName = "MyAutoStartApp";
                string appPath = @"C:\Program Files\Microsoft Office\root\Office16\WINWORD.EXE";
                AddToAutostart(appName, appPath);

                // --- ЗАВДАННЯ 6: Перевірка ---
                PrintHeader("6. Перевірка (повторний вивід)");
                ListAutostartPrograms();

                // --- ЗАВДАННЯ 7: Планувальник завдань ---
                PrintHeader("7. Список завдань (читання з Реєстру)");
                ListScheduledTasksFromRegistry();

                // --- ЗАВДАННЯ 8: Створення копії розділу у .reg файл ---
                PrintHeader("8. Експорт розділу HKCU\\Run у файл .reg");
                GenerateRegFile("backup_run.reg");

                // --- ЗАВДАННЯ 9: Створення .reg файлу асоціації ---
                PrintHeader("9. Генерація файлу асоціації .ttt");
                GenerateAssociationFile("association_ttt.reg");

            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"Критична помилка: {ex.Message}");
                Console.ResetColor();
            }

            Console.WriteLine("\nРоботу завершено. Натисніть Enter...");
            Console.ReadLine();
        }

        static void ListAutostartPrograms()
        {
            var keysToCheck = new List<(RegistryKey Root, string Path, string Source)>
            {
                (Registry.LocalMachine, @"SOFTWARE\Microsoft\Windows\CurrentVersion\Run", "HKLM Run"),
                (Registry.LocalMachine, @"SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce", "HKLM RunOnce"),
                (Registry.CurrentUser, @"SOFTWARE\Microsoft\Windows\CurrentVersion\Run", "HKCU Run"),
                (Registry.CurrentUser, @"SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce", "HKCU RunOnce")
            };

            Console.WriteLine($"{"ДЖЕРЕЛО",-15} | {"ІМ'Я ПАРАМЕТРА",-25} | {"ЗНАЧЕННЯ (ШЛЯХ)"}");
            Console.WriteLine(new string('-', 80));

            foreach (var item in keysToCheck)
            {
                try
                {
                    // WinAPI: RegOpenKeyEx
                    using (RegistryKey key = item.Root.OpenSubKey(item.Path))
                    {
                        if (key != null)
                        {
                            // WinAPI: RegEnumValue
                            foreach (string valueName in key.GetValueNames())
                            {
                                string valueData = key.GetValue(valueName)?.ToString();
                                Console.WriteLine($"{item.Source,-15} | {Truncate(valueName, 25),-25} | {valueData}");
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Помилка читання {item.Source}: {ex.Message}");
                }
            }
        }

        static void AddToAutostart(string name, string path)
        {
            try
            {
                // WinAPI: RegOpenKeyEx + RegSetValueEx
                using (RegistryKey key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run", true))
                {
                    if (key != null)
                    {
                        key.SetValue(name, path, RegistryValueKind.String);
                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.WriteLine($"[OK] Запис додано: {name} -> {path}");
                        Console.ResetColor();
                    }
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"[Error] Не вдалося додати запис: {ex.Message}");
                Console.ResetColor();
            }
        }

        static void ListScheduledTasksFromRegistry()
        {
            // Ключ, де зберігаються визначення завдань
            string tasksPath = @"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Schedule\TaskCache\Tasks";

            try
            {
                using (RegistryKey rootKey = Registry.LocalMachine.OpenSubKey(tasksPath))
                {
                    if (rootKey == null)
                    {
                        Console.WriteLine("Не вдалося відкрити гілку завдань. Перевірте права адміністратора.");
                        return;
                    }

                    string[] taskGuids = rootKey.GetSubKeyNames();
                    Console.WriteLine($"Знайдено завдань у реєстрі: {taskGuids.Length}. Виводимо перші 10:\n");

                    int count = 0;
                    foreach (string guid in taskGuids)
                    {
                        if (count >= 10) break; // Обмежимо вивід, бо їх може бути сотні

                        using (RegistryKey taskKey = rootKey.OpenSubKey(guid))
                        {
                            if (taskKey != null)
                            {
                                // Отримуємо базову інформацію (Path/URI є найбільш читабельним полем)
                                // Тригери та Дії (Actions) зберігаються у бінарному форматі або XML,
                                // парсинг яких без COM API надто складний для однієї лаби.
                                string path = taskKey.GetValue("Path")?.ToString() ?? "N/A";
                                string description = taskKey.GetValue("Description")?.ToString() ?? "(без опису)";

                                // ID завдання - це GUID (ім'я ключа)
                                Console.WriteLine($"ID: {guid}");
                                Console.WriteLine($"Шлях/Ім'я: {path}");
                                Console.WriteLine($"Опис: {description}");
                                Console.WriteLine(new string('-', 40));
                                count++;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Помилка читання завдань: {ex.Message}");
            }
        }

        static void GenerateRegFile(string fileName)
        {
            // Стратегія 1: Ручне формування тексту (без reg export)
            string keyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";

            try
            {
                using (StreamWriter sw = new StreamWriter(fileName))
                {
                    // Заголовок
                    sw.WriteLine("Windows Registry Editor Version 5.00");
                    sw.WriteLine();
                    // Назва ключа
                    sw.WriteLine($"[HKEY_CURRENT_USER\\{keyPath}]");

                    using (RegistryKey key = Registry.CurrentUser.OpenSubKey(keyPath))
                    {
                        if (key != null)
                        {
                            foreach (string valueName in key.GetValueNames())
                            {
                                object val = key.GetValue(valueName);
                                // Екранування слешів для .reg формату
                                string valueStr = val.ToString().Replace("\\", "\\\\").Replace("\"", "\\\"");

                                // Формат: "Name"="Value"
                                sw.WriteLine($"\"{valueName}\"=\"{valueStr}\"");
                            }
                        }
                    }
                }
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"[OK] Файл створено: {Path.GetFullPath(fileName)}");
                Console.ResetColor();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Помилка експорту: {ex.Message}");
            }
        }

        static void GenerateAssociationFile(string fileName)
        {
            try
            {
                using (StreamWriter sw = new StreamWriter(fileName))
                {
                    sw.WriteLine("Windows Registry Editor Version 5.00");
                    sw.WriteLine();

                    // Асоціація розширення .ttt з типом файлу tttfile
                    sw.WriteLine("[HKEY_CLASSES_ROOT\\.ttt]");
                    sw.WriteLine("@=\"tttfile\"");
                    sw.WriteLine();

                    // Команда відкриття для tttfile
                    sw.WriteLine("[HKEY_CLASSES_ROOT\\tttfile\\shell\\open\\command]");
                    // @= означає значення (Default)
                    sw.WriteLine("@=\"notepad.exe %1\"");
                }
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"[OK] Файл асоціації створено: {Path.GetFullPath(fileName)}");
                Console.WriteLine("Запустіть цей файл подвійним кліком для застосування змін.");
                Console.ResetColor();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Помилка створення файлу асоціації: {ex.Message}");
            }
        }

        static void PrintHeader(string title)
        {
            Console.WriteLine($"\n=== {title} ===");
        }

        static string Truncate(string value, int maxChars)
        {
            return value.Length <= maxChars ? value : value.Substring(0, maxChars - 3) + "...";
        }

        static bool IsAdministrator()
        {
            using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
            {
                WindowsPrincipal principal = new WindowsPrincipal(identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
        }
    }
}