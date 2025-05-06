import json
import socket
from collections import Counter


class Node:
    left = None
    right = None

    def __init__(self, char, frequency):
        self.char = char
        self.frequency = frequency


def huffman_tree(counter: Counter):
    items = sorted(counter.items(), key=lambda x: x[1], reverse=True)
    right = Node(*items.pop())
    merge = right
    while len(items):
        left = Node(*items.pop())
        merge = Node(None, right.frequency + left.frequency)
        merge.left = left
        merge.right = right
        right = merge

    return merge


def build_code_dict(node, prefix="", code_dict=None):
    if code_dict is None:
        code_dict = {}
    if node.char is not None:
        code_dict[node.char] = prefix
    else:
        build_code_dict(node.left, prefix + "0", code_dict)
        build_code_dict(node.right, prefix + "1", code_dict)
    return code_dict

def encode(text, dictionary):
    return ''.join([dictionary[char] for char in text])

def decode(text, dictionary: dict):
    decoded = ''
    inv_map = {v: k for k, v in dictionary.items()}

    i = 0
    j = 1
    while i < len(text):
        while j <= len(text):
            char = inv_map.get(text[i:j])
            if char is not None:
                decoded += char
                i = j
                j += 1
                break
            j += 1
    return decoded



if __name__ == '__main__':
    match int(input('0. Wyślij\n1. Odbierz')):
        case 0:
            address = input('Podaj adres docelowy: ')
            port = int(input('Podaj port docelowy: '))

            with open('input.txt', 'rt') as file:
                text = file.read()
            counter = Counter(text)
            tree = huffman_tree(counter)
            dictionary = build_code_dict(tree)

            encoded = encode(text, dictionary)

            # sending
            message = {
                'code_dict': dictionary,
                'data': encoded
            }


            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((address, port))
                s.sendall(json.dumps(message).encode('utf-8'))
        case 1:
            port = int(input('Nasłuchuj na porcie: '))
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.bind(('0.0.0.0', port))
                s.listen()
                conn, addr = s.accept()
                with conn:
                    print(f"Połączono z: {addr}")
                    data = conn.recv(65536)
                    msg = json.loads(data.decode('utf-8'))

                    decoded = decode(msg['data'], msg['code_dict'])
                    print('Odebrany tekst:', decoded, sep='\n')
                    with open("odebrany_plik.txt", "w", encoding="utf-8") as f:
                        f.write(decoded)

                    print("Plik został zapisany jako 'odebrany_plik.txt'")

