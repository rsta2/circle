#!/usr/bin/python
# convert binary file to C bytes array
# for embedding into other program
# (c) Alexander Belchenko, 2004-2006
# http://onembedding.com/tools/python/code/
# v.2

'''Convert binary file to C-arrays.
Will generate 2 files: file.c and file.h

Usage:
    python bin2c.py [options] file.bin

Options:
    -h, --help      - this help
    --decl=arrtype  - declaration of array type.
                      (default: 'unsigned char')
'''

import os
import sys


def main(argv=None, decl=None):
    """Main function.

    @param  argv:   list of command-line arguments
    @param  decl:   custom declaration of array type.

    @return:    utility exit code (0 == OK)
    """
    import getopt

    if argv is None:
        argv = sys.argv[1:]
    if decl is None:
        decl = 'unsigned char'

    try:
        opts, args = getopt.getopt(argv, 'h',
                                   ['help', 'decl=',
                                   ])
        for o, a in opts:
            if o in ('-h', '--help'):
                print __doc__
                return 0
            elif o == '--decl':
                decl = a

        if len(args) < 1:
            raise getopt.GetoptError, "Binary file not specified"

    except getopt.GetoptError, msg:
        print str(msg)
        print __doc__
        return 1


    bin = args[0]
    base, ext = os.path.splitext(bin)
    if ext not in ('.tsk', '.bin', '.rom'):
        print 'Expected binary file with extension either .bin or .tsk'
        return 1
        
    fbin = file(bin, 'rb')
    cfile = base + '.c'
    fcout = file(cfile, 'w')
    hfile = base + '.h'
    fhout = file(hfile, 'w')

    if args[1] is not None:
	    base = args[1]
    else:
        base = "binary_" + base
    fcout.write ('%s  %s[] = {\n' % (decl, base))
    k = 0
    count = 0
    
    while 1:
        byte = fbin.read(1)
        if byte == '':
            break;
        
        if k == 0:
            fcout.write ('\t')
        
        fcout.write('0x%02X, ' % ord(byte))
        count += 1
        k += 1
        if k == 16:     
            k = 0
            fcout.write('\n')
            
    if k != 0:
        fcout.write('\n')
            
    fcout.write('};\n')
    fcout.close()
    
    if count == 0:
        print 'File %s is empty!' % bin
        return 1
        
    fhout.write('extern %s  %s[%d];\n' % (decl, base, count))
    fhout.close()
    
    print 'Processed: %s' % bin

    return 0
#/def main


if __name__ == '__main__':
    sys.exit(main())
