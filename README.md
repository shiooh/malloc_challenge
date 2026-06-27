# malloc_challenge

## Task0
ソースコード：[malloc/malloc.c](./malloc/malloc.c)

元の malloc の実行結果は以下の通り。

| Time1<br>[ms] | Utilization1<br>[%] | Time2<br>[ms] | Utilization2<br>[%] | Time3<br>[ms] | Utilization3<br>[%] | Time4<br>[ms] | Utilization4<br>[%] | Time5<br>[ms] | Utilization5<br>[%] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 8 | 70 | 7 | 39 | 73 | 8 | 5393 | 15 | 3634 | 15 |

## Task1
ソースコード：[malloc/malloc_best_fit.c](./malloc/malloc_best_fit.c)

best_fit を適用した結果は以下の通り。

| Time1<br>[ms] | Utilization1<br>[%] | Time2<br>[ms] | Utilization2<br>[%] | Time3<br>[ms] | Utilization3<br>[%] | Time4<br>[ms] | Utilization4<br>[%] | Time5<br>[ms] | Utilization5<br>[%] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 999 | 70 | 406 | 39 | 618 | 51 | 5853 | 72 | 4156 | 72 |

Utilization が大幅に改善しており、空き領域の選び方がより適切になったことがわかる。
ただし、空き領域を見つけたらすぐに次の処理に進む first_fit と異なり、best_fit ではすべての空き領域を探しているので、実行時間 Time は悪くなっている。

## Task2
ソースコード：[malloc/malloc_first_list_bit.c](./malloc/malloc_first_list_bit.c)

first_list_bit を適用した結果は以下の通り。ただし、bit の数は 10、１つの bit が担当する空き領域のサイズの範囲 bin_range は 200 とした。

| Time1<br>[ms] | Utilization1<br>[%] | Time2<br>[ms] | Utilization2<br>[%] | Time3<br>[ms] | Utilization3<br>[%] | Time4<br>[ms] | Utilization4<br>[%] | Time5<br>[ms] | Utilization5<br>[%] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 964 | 70 | 398 | 39 | 595 | 51 | 207 | 72 | 927 | 72 |

空き領域の選び方は変わっていないので Utilization に変化はないが、実行時間は大幅に速くなっている。

なお、bin_range は小さくするほど実行時間は早くなったが、減らしすぎると my_find_list_index() でインデックスを探すときなどのコストが上がってしまい、challenge 1 などの小さな challenge ではオーバーヘッドになってしまう。

## Task3
ソースコード：[malloc/malloc.c](./malloc/malloc.c)


## Task4
ソースコード：[malloc/malloc.c](./malloc/malloc.c)




# (original)
[![Open in Cloud Shell](https://gstatic.com/cloudssh/images/open-btn.svg)](https://shell.cloud.google.com/cloudshell/editor?cloudshell_git_repo=https%3A%2F%2Fgithub.com%2Fhikalium%2Fmalloc_challenge&cloudshell_open_in_editor=malloc.c&cloudshell_workspace=malloc)

- `malloc` is the malloc challenge. Please read this doc and [malloc/malloc.c](./malloc/malloc.c) for more information.
- `visualizer/` contains a visualizer of malloc traces.

## Instruction

Your task is implement a better malloc logic in [malloc.c](./malloc/malloc.c) to improve the speed and memory usage.

## How to build & run a benchmark

```
# clone this repo
git clone https://github.com/hikalium/malloc_challenge.git

# move into malloc dir
cd malloc_challenge
cd malloc

# build
make

# run a benchmark (for score board)
make run

# run a small benchmark for tracing (NOT for score board, just for visualization and debugging purpose)
make run_trace
```

If the commands above don't work, please make sure the following packages are installed:
```
# For Debian-based OS
sudo apt install make clang
```

Alternatively, you can build and run the challenge directly by running:

```
gcc -Wall -O3 -lm -o malloc_challenge.bin main.c malloc.c simple_malloc.c
./malloc_challenge.bin
```

## Acknowledgement

This work is based on [xharaken's malloc_challenge.c](https://github.com/xharaken/step2/blob/master/malloc_challenge.c). Thank you haraken-san!

