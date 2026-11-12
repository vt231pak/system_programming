@ECHO OFF
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
chcp 65001 > nul

rem =========================================================
rem НАЛАШТУВАННЯ СКРИПТА
rem =========================================================

SET me=%~n0
SET parent=%~dp0

rem Завдання 1: Перевірка аргументів
IF "%6"=="" (
    ECHO Помилка: Пакетний файл приймає 6 обов'язкових Аргументів
    ECHO Формат: %me%.cmd ^<LogFile^> ^<SourcePath^> ^<ProcessName^> ^<ArchivePath^> ^<TargetIP^> ^<MaxLogKB^>
    ECHO Приклад: %me%.cmd lab.log C:\Work\Data notepad.exe C:\Archive 192.168.1.50 500
    GOTO :EOF
)

SET logfile=%~1
SET pathtoarchive=%~2
SET processname=%~3
SET archivedir=%~4
SET target_ip=%~5
SET maxlogkb=%~6

IF NOT EXIST "%logfile%" (
    TYPE NUL > "%logfile%"
)

rem Запис заголовка сесії
echo. >> "%logfile%"
echo ======================================================== >> "%logfile%"
echo [%date% %time%] СТАРТ СКРИПТА >> "%logfile%"
echo [%date% %time%] Файл з ім'ям %logfile% відкрито або створено. >> "%logfile%"

rem Завдання 2: Синхронізація часу
echo [%date% %time%] --- СИНХРОНІЗАЦІЯ ЧАСУ --- >> "%logfile%"
echo [%date% %time%] Старий час: %date% %time% >> "%logfile%"

net start w32time >nul 2>&1
w32tm /resync >> "%logfile%" 2>&1
if !errorlevel! EQU 0 (
    echo [%date% %time%] Час успішно синхронізовано. >> "%logfile%"
) else (
    echo [%date% %time%] ПОМИЛКА синхронізації!errorlevel! >> "%logfile%"
)

echo [%date% %time%] Новий час: %date% %time% >> "%logfile%"

rem Завдання 3: Список процесів
echo [%date% %time%] --- СПИСОК ПРОЦЕСІВ --- >> "%logfile%"
tasklist /FO CSV /NH >> "%logfile%" 2>&1

rem Завдання 4: Завершення процесу
echo [%date% %time%] --- ЗАВЕРШЕННЯ ПРОЦЕСУ %processname% --- >> "%logfile%"
tasklist /FI "IMAGENAME eq %processname%" 2>nul | find /I "%processname%" >nul
if not errorlevel 1 (
    echo [%date% %time%] Процес знайдено. Спроба завершити... >> "%logfile%"
    taskkill /IM "%processname%" /F >> "%logfile%" 2>&1
    if not errorlevel 1 (
        echo [%date% %time%] Процес успішно завершено. >> "%logfile%"
    ) else (
        echo [%date% %time%] ПОМИЛКА завершення процесу. >> "%logfile%"
    )
) else (
    echo [%date% %time%] Процес %processname% не знайдено. >> "%logfile%"
)

rem Завдання 5: Очищення TMP файлів
echo [%date% %time%] --- ОЧИЩЕННЯ ФАЙЛІВ У %pathtoarchive% --- >> "%logfile%"
if exist "%pathtoarchive%" (
    set /a del_count=0
    
    rem Видалення *.tmp
    for /r "%pathtoarchive%" %%F in (*.tmp) do (
        del /f /q "%%F" >> "%logfile%" 2>&1
        set /a del_count+=1
    )
    
    rem Видалення temp* (файлів)
    for /r "%pathtoarchive%" %%F in (temp*) do (
        if exist "%%F" (
             del /f /q "%%F" >> "%logfile%" 2>&1
             set /a del_count+=1
        )
    )
    echo [%date% %time%] Видалено файлів: !del_count! >> "%logfile%"
) else (
    echo [%date% %time%] Каталог %pathtoarchive% не існує. >> "%logfile%"
)

rem Завдання 6: Архівування
echo [%date% %time%] --- АРХІВУВАННЯ --- >> "%logfile%"

set "ts=%date:~-4%%date:~3,2%%date:~0,2%-%time:~0,2%%time:~3,2%%time:~6,2%"
set "ts=%ts: =0%"
set "zipname=%ts%.zip"

rem Перевірка шляхів
if exist "%pathtoarchive%" (
    if not exist "%archivedir%" mkdir "%archivedir%"
    
    echo [%date% %time%] Архівуємо файли з: %pathtoarchive% >> "%logfile%"
    
    rem Зберігаємо поточний шлях і переходимо в папку з файлами
    pushd "%pathtoarchive%"
    
    rem Спробуємо стиснути через TAR (Win10)
    tar --version >nul 2>&1
    if not errorlevel 1 (
        echo [%date% %time%] Використання tar >> "%logfile%"
        rem Важливо: вказуємо повний шлях до цільового архіву ("%archivedir%\%zipname%")
        tar -a -c -f "%archivedir%\%zipname%" * >> "%logfile%" 2>&1
    ) else (
        echo [%date% %time%] tar не знайдено, спроба через PowerShell >> "%logfile%"
        rem Резервний варіант через PowerShell (Compress-Archive), якщо tar не працює
        powershell -command "Compress-Archive -Path '.\*' -DestinationPath '%archivedir%\%zipname%' -Force" >> "%logfile%" 2>&1
    )
    
    rem Повертаємося у вихідну папку скрипта
    popd
    
    if exist "%archivedir%\%zipname%" (
        echo [%date% %time%] УСПІХ: Архів створено: %zipname% >> "%logfile%"
    ) else (
        echo [%date% %time%] ПОМИЛКА: Не вдалося створити архів >> "%logfile%"
    )

) else (
    echo [%date% %time%] ПОМИЛКА: Папка джерела %pathtoarchive% не існує! >> "%logfile%"
)

rem Завдання 7: Перевірка архіву за вчора
echo [%date% %time%] --- ПЕРЕВІРКА ВЧОРАШНЬОГО АРХІВУ --- >> "%logfile%"

rem Отримання вчорашньої дати через VBS
echo d = DateAdd("d", -1, Date) > "%temp%\yesterday.vbs"
echo WScript.Echo Year(d) ^& Right("0" ^& Month(d), 2) ^& Right("0" ^& Day(d), 2) >> "%temp%\yesterday.vbs"
for /f %%A in ('cscript //nologo "%temp%\yesterday.vbs"') do set "yesterday=%%A"
del "%temp%\yesterday.vbs"

if exist "%archivedir%\%yesterday%-*.zip" (
    echo [%date% %time%] Архів за %yesterday% знайдено. >> "%logfile%"
) else (
    echo [%date% %time%] УВАГА: Архів за %yesterday% ВІДСУТНІЙ. >> "%logfile%"
    call :SendEmail "Missing Archive" "Archive for %yesterday% was not found in %archivedir%"
)

rem Завдання 8: Видалення архівів старших 30 днів (через forfiles)
echo [%date% %time%] --- ВИДАЛЕННЯ СТАРИХ АРХІВІВ --- >> "%logfile%"
if exist "%archivedir%" (
    rem /D -30 вибирає файли старші 30 днів
    forfiles /P "%archivedir%" /M *.zip /D -30 /C "cmd /c del @file & echo [%date% %time%] Видалено старий архів: @file" >> "%logfile%" 2>&1
    echo [%date% %time%] Очистка завершена. >> "%logfile%"
)

rem Завдання 9: Інтернет
echo [%date% %time%] --- ПЕРЕВІРКА ІНТЕРНЕТУ --- >> "%logfile%"

rem Ping з таймаутом 1500 мс, щоб не чекати довго
ping 8.8.8.8 -n 1 -w 1500 >nul 2>&1

if !errorlevel! EQU 0 (
    echo [%date% %time%] Інтернет доступний >> "%logfile%"
) else (
    echo [%date% %time%] Інтернет ВІДСУТНІЙ >> "%logfile%"
)

rem Завдання 10: Віддалене вимкнення
echo [%date% %time%] --- ВІДДАЛЕНЕ ВИМКНЕННЯ %target_ip% --- >> "%logfile%"

ping %target_ip% -n 1 >nul
if not errorlevel 1 (

    echo [%date% %time%] Підключаюсь до %target_ip% >> "%logfile%"
    net use \\%target_ip% /user:labuser StrongPass123 >> "%logfile%" 2>&1

    if not errorlevel 1 (
        echo [%date% %time%] Надсилаю shutdown на %target_ip% >> "%logfile%"
        shutdown /m \\%target_ip% /s /f /t 0 >> "%logfile%" 2>&1
    ) else (
        echo [%date% %time%] Помилка підключення до %target_ip% >> "%logfile%"
    )

    rem Відключаємо мережеву сесію
    net use \\%target_ip% /delete >nul 2>&1

) else (
    echo [%date% %time%] Комп'ютер %target_ip% недоступний >> "%logfile%"
)


rem Завдання 11: Комп'ютери в мережі
echo [%date% %time%] --- КОМП'ЮТЕРИ В МЕРЕЖІ --- >> "%logfile%"
rem net view часто не працює на Win10, використовуємо arp як резерв
net view >> "%logfile%" 2>&1
if errorlevel 1 arp -a >> "%logfile%"

rem Завдання 12: Перевірка списку IP
echo [%date% %time%] --- ПЕРЕВІРКА IP З СПИСКУ --- >> "%logfile%"
if exist "ipon.txt" (
    for /f "usebackq tokens=*" %%A in ("ipon.txt") do (
        ping %%A -n 1 >nul
        if errorlevel 1 (
            echo [%date% %time%] Хост %%A недоступний. >> "%logfile%"
            call :SendEmail "Host Down" "Host %%A is not responding."
        ) else (
             echo [%date% %time%] Хост %%A працює. >> "%logfile%"
        )
    )
) else (
     echo [%date% %time%] Файл ipon.txt не знайдено. >> "%logfile%"
)

rem Завдання 13: Розмір логу
echo [%date% %time%] --- ПЕРЕВІРКА РОЗМІРУ ЛОГУ --- >> "%logfile%"
for %%F in ("%logfile%") do set size=%%~zF
set /a sizeKB=%size%/1024
echo Поточний розмір: %sizeKB% KB. Ліміт: %maxlogkb% KB. >> "%logfile%"
if %sizeKB% GTR %maxlogkb% (
    echo [%date% %time%] ПЕРЕВИЩЕННЯ ЛІМІТУ ЛОГУ! >> "%logfile%"
    call :SendEmail "Log Size Alert" "Log file %logfile% is too big: %sizeKB% KB."
)

rem Завдання 14: Місце на дисках
echo [%date% %time%] --- МІСЦЕ НА ДИСКАХ --- >> "%logfile%"
wmic logicaldisk get caption,size,freespace >> "%logfile%"

rem Завдання 15: systeminfo
echo [%date% %time%] --- ЗБЕРЕЖЕННЯ SYSTEMINFO --- >> "%logfile%"
rem Формування імені systeminfo-YYYYMMDD-HHMMSS.txt
set "sysfile=systeminfo-%ts%.txt"
systeminfo > "%sysfile%"
echo [%date% %time%] Інформація збережена у %sysfile% >> "%logfile%"

GOTO :EOF

rem =========================================================
rem ФУНКЦІЯ ВІДПРАВКИ EMAIL (POWERSHELL)
rem =========================================================
:SendEmail
SET "SUBJ=%~1"
SET "BODY=%~2"
echo [%date% %time%] [EMAIL] Спроба відправки: %SUBJ% >> "%logfile%"

rem УВАГА: Для Gmail потрібно використовувати App Password, звичайний пароль не підійде!
rem Заповни ці поля своїми даними для тесту:
SET "SMTP_SERVER=smtp.gmail.com"
SET "SMTP_PORT=587"
SET "EMAIL_FROM=from@gmail.com"
SET "EMAIL_TO=to@gmail.com"
SET "EMAIL_USER=user@gmail.com"
SET "EMAIL_PASS=pass"

powershell -Command "Send-MailMessage -SmtpServer '%SMTP_SERVER%' -Port %SMTP_PORT% -UseSsl -Credential (New-Object System.Management.Automation.PSCredential('%EMAIL_USER%',(ConvertTo-SecureString '%EMAIL_PASS%' -AsPlainText -Force))) -From '%EMAIL_FROM%' -To '%EMAIL_TO%' -Subject '%SUBJ%' -Body '%BODY%'" >> "%logfile%" 2>&1

EXIT /B