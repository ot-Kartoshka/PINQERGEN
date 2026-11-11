#include "Errors.hpp"

std::string_view error_to_string(Error err) {
    switch (err) {
    case Error::ParseError:         return "Помилка розбору аргументів командного рядка.";
    case Error::UnknownGenerator:   return "Невідомий датчик. Див. --list.";
    case Error::InvalidLength:      return "Некоректна довжина послідовності.";
    case Error::InvalidAlpha:       return "Некоректний рівень значимості (має бути в (0, 1)).";
    case Error::InvalidSegments:    return "Некоректна кількість відрізків для тесту однорідності.";
    case Error::FileReadError:      return "Не вдалося прочитати файл.";
    case Error::FileWriteError:     return "Не вдалося записати файл.";
    case Error::EmptySequence:      return "Послідовність порожня -- нема що перевіряти.";
    case Error::NotEnoughData:      return "Замало даних для коректного застосування критерію (n < 50 або v_j < 5).";
    default:                        return "Невідома помилка.";
    }
}
