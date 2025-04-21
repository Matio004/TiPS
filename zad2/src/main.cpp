#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define C 0x43

#define BLOCK_SIZE 128
#define TIMEOUT 10000
#define MAX_RETRIES 10

HANDLE hSerial;

bool useCRC = false;
const uint16_t crc16tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

uint8_t calculateChecksum(const std::vector<uint8_t>& data) {
    uint8_t sum = 0;
    for (const auto& byte : data) {
        sum += byte;
    }
    return sum;
}

uint16_t calculateCRC16(const std::vector<uint8_t>& data) {
    uint16_t crc = 0;
    for (const auto& byte : data) {
        crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ byte) & 0xFF];
    }
    return crc;
}

int readWithTimeout(std::vector<uint8_t>& buffer, size_t count) {
    DWORD bytesRead = 0;
    buffer.resize(count);

    if (!ReadFile(hSerial, buffer.data(), static_cast<DWORD>(count), &bytesRead, NULL)) {
        return -1;
    }

    return static_cast<int>(bytesRead);
}

int readByteWithTimeout(uint8_t& byte) {
    std::vector<uint8_t> buffer;
    int result = readWithTimeout(buffer, 1);
    if (result == 1) {
        byte = buffer[0];
    }
    return result;
}

int writeAll(const std::vector<uint8_t>& buffer) {
    DWORD bytesWritten = 0;

    if (!WriteFile(hSerial, buffer.data(), buffer.size(), &bytesWritten, NULL)) {
        return -1;
    }

    return static_cast<int>(bytesWritten);
}

// Zapis pojedynczego bajtu do portu
int writeByte(uint8_t byte) {
    std::vector buffer = {byte};
    return writeAll(buffer);
}

void configPorts(const std::string& port) {
    hSerial = CreateFileA(port.c_str(),GENERIC_WRITE | GENERIC_READ, 0,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);
    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = TIMEOUT;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = TIMEOUT;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);
}

bool receiveFile(const std::string& path) {
    std::ofstream file(path, std::ios::binary);

    uint8_t expectedBlock = 1;
    std::vector<uint8_t> buffer(BLOCK_SIZE);
    uint8_t headerByte;
    uint8_t blockNumber;
    uint8_t blockNumberComplement;
    bool receiving = false;

    for (int i = 0; i < 6; ++i) {
        writeByte(useCRC ? C : NAK);

        if (readByteWithTimeout(headerByte) > 0) {
            if (headerByte == SOH) {
                // Otrzymaliśmy SOH, rozpoczynamy odbieranie danych
                receiving = true;
                break;
            } else if (headerByte == EOT) {
                // Przypadek pustego pliku
                writeByte(ACK);
                file.close();
                return true;
            }
        }
    }
    if (!receiving) {
        return false;
    }
    while (receiving) {
        if (headerByte == SOH) {
            if (readByteWithTimeout(blockNumber) <= 0 || readByteWithTimeout(blockNumberComplement) <= 0) {
                if (useCRC) writeByte(C);
                else writeByte(NAK);
                continue;
            }
           // Weryfikacja numeru bloku i jego dopełnienia
            if (blockNumber + blockNumberComplement != 255) {
                if (useCRC) writeByte(C);
                else writeByte(NAK);
                continue;
            }

        std::vector<uint8_t> dataBlock;
        int dataResult = readWithTimeout(dataBlock, BLOCK_SIZE);
        if (dataResult != BLOCK_SIZE) {
            if (useCRC) writeByte(C); else writeByte(NAK);
            continue;
        }

        bool checksumValid;

        if (useCRC) {
            // Odczyt 2 bajtów CRC
            uint8_t crcHigh, crcLow;
            if (readByteWithTimeout(crcHigh) <= 0 || readByteWithTimeout(crcLow) <= 0) {
                std::cerr << "Błąd odczytu CRC" << std::endl;
                writeByte(NAK);
                continue;
            }

            uint16_t receivedCRC = (static_cast<uint16_t>(crcHigh) << 8) | crcLow;
            uint16_t calculatedCRC = calculateCRC16(dataBlock);

            checksumValid = (receivedCRC == calculatedCRC);
        } else {
            // Odczyt 1 bajtu sumy kontrolnej
            uint8_t receivedChecksum;
            if (readByteWithTimeout(receivedChecksum) <= 0) {
                std::cerr << "Błąd odczytu sumy kontrolnej" << std::endl;
                writeByte(NAK);
                continue;
            }

            uint8_t calculatedChecksum = calculateChecksum(dataBlock);
            checksumValid = (receivedChecksum == calculatedChecksum);
        }

        if (!checksumValid) {
            writeByte(NAK);
            continue;
        }

        // Obsługa duplikatu bloku
                if (blockNumber == (expectedBlock - 1) && expectedBlock > 1) {
                    // Potwierdzenie duplikatu bloku, ale bez zapisywania do pliku
                    writeByte(ACK);
                } else if (blockNumber == expectedBlock) {
                    // Zapis danych do pliku
                    file.write(reinterpret_cast<const char*>(dataBlock.data()), dataBlock.size());
                    writeByte(ACK);
                    expectedBlock++;
                } else {
                    // Nieoczekiwany numer bloku
                    writeByte(NAK);
                    continue;
                }

                // Odczyt nagłówka następnego pakietu
                if (readByteWithTimeout(headerByte) <= 0) {
                    writeByte(NAK);
                    continue;
                }

                if (headerByte == EOT) {
                    // Koniec transmisji
                    writeByte(ACK);
                    receiving = false;
                } else if (headerByte != SOH) {
                    // Nieoczekiwany znak kontrolny
                    writeByte(NAK);
                    continue;
                }
        }
        else if (headerByte == EOT) {
            // Koniec transmisji
            writeByte(ACK);
            receiving = false;
        } else {
            // Nieoczekiwany znak kontrolny
            writeByte(NAK);

            // Próba ponownego odczytu nagłówka
            if (readByteWithTimeout(headerByte) <= 0) {
                continue;
            }
        }
    }

    file.close();
    return true;
}

bool sendFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }

        uint8_t blockNumber = 1;
        std::vector<uint8_t> buffer(BLOCK_SIZE);
        std::vector<uint8_t> packet;
        uint8_t response;
        bool waitingForInitiation = true;
        int retries = 0;

        // Oczekiwanie na NAK lub C od odbiornika, aby rozpocząć transmisję
        while (waitingForInitiation && retries < MAX_RETRIES) {
            if (readByteWithTimeout(response) > 0) {
                if (response == NAK) {
                    waitingForInitiation = false;
                    useCRC = false;
                } else if (response == C) {
                    waitingForInitiation = false;
                    useCRC = true;
                }
            } else {
                retries++;
            }
        }

        if (waitingForInitiation) {
            return false;
        }

        // Rozpoczęcie wysyłania bloków
        while (true) {
            file.read(reinterpret_cast<char*>(buffer.data()), BLOCK_SIZE);
            size_t bytesRead = file.gcount();

            if (bytesRead == 0) {
                // Koniec pliku, wysyłamy EOT
                bool eotAcked = false;
                retries = 0;

                while (!eotAcked && retries < MAX_RETRIES) {
                    writeByte(EOT);

                    if (readByteWithTimeout(response) > 0) {
                        if (response == ACK) {
                            eotAcked = true;
                        }
                    }

                    retries++;
                }

                if (!eotAcked) {
                    return false;
                }

                break;
            }

            // Uzupełnienie bloku znakami SUB (0x1A), jeśli potrzeba
            if (bytesRead < BLOCK_SIZE) {
                std::fill(buffer.begin() + bytesRead, buffer.end(), 0x1A);
            }

            bool blockAcked = false;
            retries = 0;

            while (!blockAcked && retries < MAX_RETRIES) {
                // Przygotowanie pakietu
                if (useCRC) {
                    // SOH + blockNumber + ~blockNumber + dane + CRC (2 bajty)
                    packet.resize(BLOCK_SIZE + 5);
                    packet[0] = SOH;
                    packet[1] = blockNumber;
                    packet[2] = 255 - blockNumber;
                    std::copy(buffer.begin(), buffer.end(), packet.begin() + 3);

                    uint16_t crc = calculateCRC16(buffer);
                    packet[BLOCK_SIZE + 3] = (crc >> 8) & 0xFF; // MSB
                    packet[BLOCK_SIZE + 4] = crc & 0xFF;        // LSB
                } else {
                    // SOH + blockNumber + ~blockNumber + dane + suma kontrolna (1 bajt)
                    packet.resize(BLOCK_SIZE + 4);
                    packet[0] = SOH;
                    packet[1] = blockNumber;
                    packet[2] = 255 - blockNumber;
                    std::copy(buffer.begin(), buffer.end(), packet.begin() + 3);
                    packet[BLOCK_SIZE + 3] = calculateChecksum(buffer);
                }

                // Wysyłanie pakietu
                writeAll(packet);

                // Oczekiwanie na ACK lub NAK
                if (readByteWithTimeout(response) > 0) {
                    if (response == ACK) {
                        blockAcked = true;
                        blockNumber++;
                    } else if (response == NAK) {
                    } else if (response == CAN) {
                        return false;
                    }
                }

                retries++;
            }

            if (!blockAcked) {
                return false;
            }
        }

        return true;
    }

int main(int argc, char *argv[]) {
    configPorts("COM1");
    if (argc < 3 || argc > 4) {
        return -1;
    }
    if (argc == 4) {
        if (strcmp(argv[3], "0") == 0) {
            useCRC = false;
        }
        else if (strcmp(argv[3], "1") == 0) {
            useCRC = true;
        }
    }

    if (strcmp(argv[1], "R") == 0) {
        bool result = receiveFile(argv[2]);
        if (result) {
            std::cout << "Poprawnie odebrano plik!" << std::endl;
        }
        else {
            std::cout << "Niepoprawnie odebrano plik!" << std::endl;
        }
    }
    else if (strcmp(argv[1], "S") == 0) {
        bool result = sendFile(argv[2]);
        if (result) {
            std::cout << "Poprawnie wysłano plik!" << std::endl;
        }
        else {
            std::cout << "Niepoprawnie wysłano plik!" << std::endl;
        }
    }
    return 0;
}
