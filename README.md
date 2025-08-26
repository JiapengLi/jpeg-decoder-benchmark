


## Run tool.py

```bash
conda env list
conda create -n jpgd python=3.12
conda activate jpgd
pip install pillow fire numpy
python tool.py gen --W 320 --H 240 --num 5
```


## Build

```bash
gcc -I tjpgd/src -I zjpgd/zjpgd main.c tjpgd/src/tjpgd.c zjpgd/zjpgd/zjpgd.c -o jpgdtest
```

## Test

```bash
./test.sh
```

[Results](./results.csv)



