import os
import sys

def file_to_c_array(file_path, var_name):
    if not file_path or not os.path.exists(file_path):
        return f"const unsigned int {var_name}_len = 0;\nconst unsigned char {var_name}_data[] = {{ 0 }};\n\n"

    with open(file_path, 'rb') as f:
        data = f.read()
    
    lines = []
    lines.append(f"const unsigned int {var_name}_len = {len(data)};")
    lines.append(f"const unsigned char {var_name}_data[] = {{")
    
    # Write in chunks of 16 bytes
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hex_bytes = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hex_bytes},")
    lines.append("};\n")
    return "\n".join(lines)

def main():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    dll_path = os.path.join(root, 'build', 'text-scale.dll')
    effect_path = os.path.join(root, 'data', 'text_scale.effect')
    locale_path = os.path.join(root, 'data', 'locale', 'en-US.ini')
    output_h = os.path.join(root, 'installer', 'payload.h')

    pthread_candidates = [
        os.path.join(root, 'libwinpthread-1.dll'),
        r'C:\Users\Dell\mingw64\bin\libwinpthread-1.dll',
    ]
    pthread_path = None
    for p in pthread_candidates:
        if os.path.exists(p):
            pthread_path = p
            break

    os.makedirs(os.path.dirname(output_h), exist_ok=True)

    print("Generating installer/payload.h...")
    with open(output_h, 'w', encoding='utf-8') as out:
        out.write("#pragma once\n\n")
        out.write(file_to_c_array(dll_path, "PAYLOAD_DLL"))
        out.write(file_to_c_array(pthread_path, "PAYLOAD_PTHREAD"))
        out.write(file_to_c_array(effect_path, "PAYLOAD_EFFECT"))
        out.write(file_to_c_array(locale_path, "PAYLOAD_LOCALE"))

    print(f"Generated {output_h} successfully.")

if __name__ == '__main__':
    main()
