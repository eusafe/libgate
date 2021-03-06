#ifndef _EV_H
#define _EV_H

//  Нажата кнопка открывания двери 
#define GP_EV_OPEN 00h
// Ключ не найден в банке ключей
#define GP_EV_NOFOUND 02h
// Ключ найден в банке ключей
#define GP_EV_FOUND 04h
// Доступ ключу запрещен, несовпадение расписаний
#define GP_EV_SCHED_NOTIME 06h
// Команда открывания оператором по сети
#define GP_EV_ 08h
// Доступ (ключ/кнопка/оператор) запрещен, дверь заблокирована
#define GP_EV_TOKEN_LOCKED 0Ah
// Дверь взломана
#define GP_EV_DOOR_BROKE 0Ch
// Дверь оставлена открытой после прохода (ключ/кнопка/оператор)
#define GP_EV_DOOR_OPENED 0Eh
// Проход состоялся (ключ/кнопка/оператор)
#define GP_EV_ 010h
// Сработал датчик 1
#define GP_EV_DAT1 012h
// Сработал датчик 2
#define GP_EV_DAT2 013h
// Перезагрузка контроллера
#define GP_EV_REBOOT 014h
// Залипание кнопки открывания
#define GP_EV_ 016h
// Подтверждение проезда 
#define GP_EV_ 018h
// Запрет попытки повторного прохода
#define GP_EV_ 01Ah
// Ворота открыты
#define GP_EV_ 01Ch
// Ворота закрыты
#define GP_EV_ 01Dh
// Доступ ключу запрещен. Лимит исчерпан.
#define GP_EV_ 01Eh
// Введен неверный код
#define GP_EV_ 020h
// Успешное подтверждение прохода
#define GP_EV_ 022h
// Неверная карта подтверждения
#define GP_EV_ 024h
// Истек таймаут подтверждения
#define GP_EV_ 026h
// Проход не был совершен
#define GP_EV_ 028h
// Дверь закрыта
#define GP_EV_ 02Ah
// Постановка на охрану состоялась
#define GP_EV_ 02Ch
// Постановка на охрану запрещена
#define GP_EV_ 02Dh
// Снятие с охраны состоялось
#define GP_EV_ 02Eh
// Снятие с охраны запрещено
#define GP_EV_ 02Fh
// Короткое замыкание охранного шлейфа
#define GP_EV_ 030h
// Обрыв охранного шлейфа
#define GP_EV_ 031h
// Сброс тревоги
#define GP_EV_ 032h
// Залипание на GND контакта считывателя DATA0 
#define GP_EV_ 034h
// Залипание на GND контакта считывателя DATA1 
#define GP_EV_ 036h

#endif /* _EV_H */
