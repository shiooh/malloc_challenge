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
ソースコード：[malloc/malloc_ummap.c](./malloc/malloc_ummap.c)

`my_free()`で、新しく得た空き領域のポインタが `mmap_from_system()` で追加したページのポインタと一致していたら、free list に追加するのではなくシステムに ummap するようにした。

`mmap_from_system()` で得るページのポインタは 4096 の倍数で、かつ各ページのサイズは 4096 よりも小さいことから
```
if((uintptr_t)metadata % 4096 == 0 && metadata->size + sizeof(my_metadata_t) == 4096)
```
という条件式が必要十分だと判断した。

ただし、現時点では 空き領域の結合を行っていないので、ummap はほぼ起こらない。

## Task4
ソースコード：[malloc/malloc_merge_to_right.c](./malloc/malloc_merge_to_right.c)

`my_free()`で、新しく得た空き領域の右側の領域が空き領域だったら、2つを結合するようにした。
具体的には、右側の領域のメタデータ`right_metadata`を計算し、free_head_list をすべて精査して一致するポインタが見つかったら、そのポインタは free_head_list から削除して、もとの空き領域のメタデータ`metadata` のサイズを更新した。

適用した結果は以下の通り。

| Time1<br>[ms] | Utilization1<br>[%] | Time2<br>[ms] | Utilization2<br>[%] | Time3<br>[ms] | Utilization3<br>[%] | Time4<br>[ms] | Utilization4<br>[%] | Time5<br>[ms] | Utilization5<br>[%] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 620 | 70 | 362 | 39 | 342 | 51 | 774 | 79 | 682 | 78 |

Utilization はわずかに、実行時間は全体的にそこそこ向上したが、challenge 4 では実行時間が悪化してしまった。

右側の領域のメタデータが空き領域か判断するのに free_head_list をすべて精査するのがオーバーヘッドになるので、メタデータが空き領域を指しているか否かの情報を追加し、また free_head_list から削除するために、prev_metadata を素早く得たいので、双方向リストにするなどの工夫の余地が残る。


## 発展課題
ソースコード：[malloc/malloc_merge_to_both_side.c](./malloc/malloc_merge_to_both_side.c)

上記の考察を踏まえ、 metadata のメンバー is_free を作り、メタデータが空き領域か簡単に判定できるようにした。
is_free の更新は、バグを防ぐために free_list を更新する Helper 関数内だけで行うようにした。

また、空き領域, 使用中領域を合わせたすべての領域をアドレス順に並べた双方向リスト neighbour を作成した。そうすると、右領域の計算が簡単になる上、左領域も free_head_list をすべて精査することなく O(1) で得られるようになった。

これらを用いて、右領域と左領域両方について、空き領域が隣接していたら結合する仕組みを作った。

ただし、それぞれの結合では metadata の free_list 上での prev ポインタを計算する my_find_prev_metadata() を呼び出している。
この関数は最悪 O(N) ,(Nは領域の数) の計算量がかかるので、これがボトルネックになっている。

free_head_list も双方向リストにすると解決できそうだが、まだ実装できていない。

適用した結果は以下の通り。
| Time1<br>[ms] | Utilization1<br>[%] | Time2<br>[ms] | Utilization2<br>[%] | Time3<br>[ms] | Utilization3<br>[%] | Time4<br>[ms] | Utilization4<br>[%] | Time5<br>[ms] | Utilization5<br>[%] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 180 | 60 | 82 | 22 | 55 | 33 | 75 | 84 | 78 | 80 |

実行時間がけた違いに短くなり、計算量を減らせていることが確認できる。

また、データ数, データサイズが大きい後半のチャレンジでは、Utilization が今までで最も良くなっており、領域結合のメリットが発揮できていることが確認できる。

ただし、前半のチャレンジでは Utilization が悪化している。これは、メンバを追加したことで構造体 my_metadata_t のサイズが大きくなり、純粋な空き領域が圧迫されたことが原因だと予想する。

## まとめ
| Name | Time1<br>[ms] | Utilization1<br>[%] | Time2<br>[ms] | Utilization2<br>[%] | Time3<br>[ms] | Utilization3<br>[%] | Time4<br>[ms] | Utilization4<br>[%] | Time5<br>[ms] | Utilization5<br>[%] |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| simple | 8 | 70 | 7 | 39 | 73 | 8 | 5393 | 15 | 3634 | 15 |
| best_fit | 999 | 70 | 406 | 39 | 618 | 51 | 5853 | 72 | 4156 | 72 |
| first_list_bit | 964 | 70 | 398 | 39 | 595 | 51 | 207 | 72 | 927 | 72 |
| merge_to_right | 620 | 70 | 362 | 39 | 342 | 51 | 774 | 79 | 682 | 78 |
| merge_to_bothside | 180 | 60 | 82 | 22 | 55 | 33 | 75 | 84 | 78 | 80 |

## 疑問
map した領域同士をまたいで左右結合しようとするとうまくいかなかった。なぜ？

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

