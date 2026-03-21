# Convert PPM (P6) to PAM (P7) format
def p6_to_p7(input_file, output_file):
    with open(input_file, "rb") as f:
        def get_token():
            token = b""
            while True:
                char = f.read(1)
                if not char: 
                    break
                if char == b"#": # Skip comment lines
                    f.readline()
                    continue
                if char.isspace():
                    if token: return token
                    continue
                token += char
            return token

        magic = get_token()
        if magic != b"P6":
            raise ValueError("Input is not a valid P6 PPM file.")
        
        width = int(get_token())
        height = int(get_token())
        maxval = int(get_token())
        
        rgb_data = f.read()

    pam_header = (
        f"P7\n"
        f"WIDTH {width}\n"
        f"HEIGHT {height}\n"
        f"DEPTH 4\n"
        f"MAXVAL {maxval}\n"
        f"TUPLTYPE RGB_ALPHA\n"
        f"ENDHDR\n"
    ).encode('ascii')

    # Just add maximum alpha value to each pixel
    alpha_byte = maxval.to_bytes(1, 'big')
    rgba_data = bytearray()
    
    for i in range(0, len(rgb_data), 3):
        rgba_data.extend(rgb_data[i:i+3])
        rgba_data.extend(alpha_byte)

    with open(output_file, "wb") as f:
        f.write(pam_header)
        f.write(rgba_data)

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python ppm_to_pam.py <input.ppm> <output.pam>")
        sys.exit(1)
    
    p6_to_p7(sys.argv[1], sys.argv[2])
