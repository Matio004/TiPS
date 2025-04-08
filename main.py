from random import sample

from zad1 import encoding, check_encoded, decoding

def main():
    with open('./message.txt', 'r') as file:
        text = file.read()

    print('Z pliku pobrano tekst:\n')
    print(text)

    encoded = encoding(text)
    print('\nTekst zakodowany\n')

    print('Uszkadzanie:')
    match int(input('0. Uszkodź automatycznie\n1. Uszkodź ręcznie\nWybór: ')):
        case 0:
            print('\nUszkadzanie automatyczne:')
            for i in range(len(encoded)):
                b0, b1 = sample(range(8), 2)

                encoded[i][7 - b0] ^= 1
                encoded[i][7 - b1] ^= 1

                print(f'Dla znaku id: {i}, szkodzono losowo wybrane bity: {b0}, {b1}')

        case 1:
            print('\nUszkadzanie ręczne')
            b0, b1 = [int(i) for i in input('Podaj dwa bity do uszkodzenia(od 0 do 7): ').split()]
            encoded[0][7 - b0] ^= 1
            encoded[0][7 - b1] ^= 1
    print('\nTekst uszkodzony:', decoding(encoded), sep='\n')
    print('\nNaprawa i odkodowywanie\n')

    decoded = decoding(check_encoded(encoded))
    print(decoded)

    with open('./transmitted.txt', 'w') as file:
        file.write(decoded)

if __name__ == "__main__":
    main()
