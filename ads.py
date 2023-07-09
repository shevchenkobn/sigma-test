import sys

ad_msg = 'advertise'
noop_msg = 'does not matter'
no_msg = 'do not advertise'


def main():
    inf = sys.stdin
    outf = sys.stdout

    count = int(inf.readline())
    for _ in range(count):
        before, after, ad = (int(d) for d in inf.readline().split(' '))
        total = before + ad
        if total < after:
            msg = ad_msg
        elif total > after:
            msg = no_msg
        else:
            msg = noop_msg
        print(msg, file=outf)

    inf.close()
    outf.close()


main()
