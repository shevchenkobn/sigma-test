import sys


def get_ints(s):
    return (int(d) for d in s.split(' '))


def get_dist(x1, y1, x2, y2):
    return ((x2 - x1)**2 + (y2 - y1)**2)**0.5


def main():
    inf = sys.stdin
    outf = sys.stdout

    n, t = get_ints(inf.readline())
    path = []  # [x, y, t]
    for _ in range(n):
        path.append(list(get_ints(inf.readline())))

    real_dist = 0
    path[0].append(0)
    for i in range(1, len(path)):
        curr, prev = path[i], path[i - 1]
        dist = get_dist(prev[0], prev[1], curr[0], curr[1])
        real_dist += dist

    last_gps, last_real = path[0], path[-1]
    gps_path = [last_gps]
    gps_dist = 0

    i = 1
    t_gps = t
    t_max = last_real[2]
    while t_gps <= t_max:
        curr_real = path[i]
        t_i = curr_real[2]
        if t_gps > t_i:
            i += 1
            continue

        if t_gps < t_i:
            prev_real = path[i - 1]
            ratio = (t_gps - prev_real[2]) / (t_i - prev_real[2])
            next_gps = [prev_real[0] + (curr_real[0] - prev_real[0]) * ratio,
                        prev_real[1] + (curr_real[1] - prev_real[1]) * ratio]
        else:
            next_gps = curr_real[:2]
        gps_path.append(next_gps)
        gps_dist += get_dist(last_gps[0], last_gps[1], next_gps[0], next_gps[1])

        t_gps += t
        last_gps = next_gps

    gps_dist += get_dist(last_gps[0], last_gps[1], last_real[0], last_real[1])

    print((real_dist - gps_dist) / real_dist * 100, file=outf)

    inf.close()
    outf.close()


main()
