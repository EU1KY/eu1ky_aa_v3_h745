'''
    This Python script is used to merge two Intel hex files into a single binary file
    Usage: python hexfile.py outfile hexfile1 hexfile2
'''

import traceback
import sys

if __name__ == '__main__':
    try:
        from intelhex import IntelHex
    except:
        try:
            import subprocess
            subprocess.check_call([sys.executable, '-m', 'pip', 'install', 'intelhex', '--user'])
            from intelhex import IntelHex
        except:
            traceback.print_exc()
            print("\n*** Please try manually with raised privileges: 'sudo python -m pip install intelhex'\n")
            sys.exit(1)

    if len(sys.argv) >= 4:
        binfile  = sys.argv[1]
        hexfile1 = sys.argv[2]
        hexfile2 = sys.argv[3]
    else:
        print('Usage: python hexfile.py outfile hexfile1 hexfile2')
        sys.exit(1)

    try:
        c7 = IntelHex(hexfile1)
        c4 = IntelHex(hexfile2)
        c7.merge(c4, overlap='ignore')
        fout = open(binfile, 'wb')
        c7.tobinfile(fout)
        fout.close()
        print('Combined image file ' + binfile + ' created')
    except:
        traceback.print_exc()
        sys.exit(1)
