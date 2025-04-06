import numpy

H = numpy.array(
    [
        [1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0],
        [1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0],
        [1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0],
        [0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0],
        [1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0],
        [1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0],
        [0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0],
        [1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1]
    ], dtype=numpy.int64
)
print(H[:, :8])
def encoding(text):
    result = []
    for char in text:
        data_bit = [int(bit) for bit in format(ord(char), '08b')]
        check_bit = (numpy.dot(H[:, :8], data_bit) % 2).tolist()
        result.append(data_bit + check_bit)
    return result

def check_encoded(encoded_text):
    result = []
    for char in encoded_text:
        syndrome = numpy.dot(H, char) % 2
        if any(syndrome):
            for i in range(16):
                char[i] = 1 - char[i]
                break
            result.append(char[:8])
        return result

def decoding(decoded_text):
    result = []
    for byte in decoded_text:
        char = chr(int(''.join(map(str,byte)), 2))
        result.append(char)
    return ''.join(result)




