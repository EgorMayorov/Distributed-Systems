## Задание 1
#### Разработка программы, использующей древовидный маркерный алгоритм для прохождения всеми процессами критических секций

Используется компилятор MPI с расширением ULFM.

Компиляция: ```/path/to/mpi/compiler/bin/mpic++ task_1.cpp -o task1```

Выполнение: ```/path/to/mpi/compiler/bin/mpirun -n 16 --map-by :OVERSUBSCRIBE task1 <marker_process_number>```

## Задание 2
#### Отказоустойчивая параллельная версия программы для сортировки данных

Используется компилятор MPI с расширением ULFM.

Компиляция: ```/path/to/mpi/compiler/bin/mpicc task_2.c -o task2```

Выполнение: ```/path/to/mpi/compiler/bin/mpirun -n <process_num + 1> --with-ft ulfm --map-by :OVERSUBSCRIBE task2 <array_size>```
