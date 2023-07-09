import sys
import math


def get_ints(s):
    return (int(d) for d in s.split(' '))


def to_int(f):
    # return round(f)
    return math.floor(f) if f < 0 else math.ceil(f)  # if f < 0 else round(f)
    # return f


def get_dist(x1, y1, x2, y2):
    # return math.sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1))
    # return math.pow((x2 - x1)**2 + (y2 - y1)**2, 0.5)
    # return math.dist((x1, y1), (x2, y2))
    # return math.dist((x1, y1), (x2, y2))
    # return math.hypot(x2 - x1, y2 - y1)
    return ((x2 - x1)**2 + (y2 - y1)**2)**0.5


def main():
    inf = sys.stdin
    outf = sys.stdout

    row_count, step = get_ints(inf.readline())
    path = []  # [x, y, t, d]
    for _ in range(row_count):
        path.append(list(get_ints(inf.readline())))

    real_dist = 0
    path[0].append(0)
    for i in range(1, len(path)):
        curr, prev = path[i], path[i - 1]
        dist = get_dist(prev[0], prev[1], curr[0], curr[1])
        curr.append(dist)
        real_dist += dist

    gps_path = []  # [x, y, t, d]
    gps_dist = 0
    i = 0
    for s in range(0, path[-1][2], step):
        while path[i][2] < s:
            i += 1
        next_real = path[i]
        if i == s:
            curr = list(next_real)
        else:
            curr_real = path[i - 1] if i > 0 else [0, 0, 0, 0]
            prev = path[-1] if s > 0 else [0, 0, 0, 0]  # s / step - 1

            ratio = (s - curr_real[2]) / next_real[2] if next_real[2] != 0 else 1
            curr = [curr_real[0] + to_int(ratio * (next_real[0] - curr_real[0])),
                    curr_real[1] + to_int(ratio * (next_real[1] - curr_real[1])),
                    s,
                    0]
            curr[3] = get_dist(prev[0], prev[1], curr[0], curr[1])
            # dist = next_real[3] * s / next_real[2]
        gps_path.append(curr)
        gps_dist += curr[3]

    real_last, gps_last = path[-1], gps_path[-1]
    gps_dist += get_dist(real_last[0], real_last[1], gps_last[0], gps_last[1])

    # print(gps_dist, real_dist, gps_path)
    print((real_dist - gps_dist) / real_dist * 100, file=outf)

    inf.close()
    outf.close()


main()
