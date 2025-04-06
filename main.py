from zad1 import encoding, check_encoded, decoding

def main():
    text = "To jest tekst"
    print("Tekst originalnie:", text)

    encoded = encoding(text)
    print("\nPo zakodowaniu:")
    for block in encoded:
        print(block)

    encoded[1][0] ^= 1
    print("\nPo flipie:")
    for block in encoded:
        print(block)

    corrected = check_encoded(encoded)
    print("\nPoprawione po flipie:")
    for block in corrected:
        print(block)

    decoded = decoding(corrected)
    print("\nOdkodowane:", decoded)

if __name__ == "__main__":
    main()
