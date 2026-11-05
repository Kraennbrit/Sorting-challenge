# Sorting-challenge

Competição para organização de uma lista em tempo mínimo com o objetivo de realizar a Implementação e otimização de algoritmos de ordenação em linguagem C para ordenar uma base volumosa de dados no menor tempo de processamento possível. <br />
O algoritmo ultilizado foi **Counting Sort** de Complexidade: <br />

      Temporal: O(m+n) em todos os casos; <br />
      Espaço: O(n+m) que não foi um requisito; <br />

Quando o intervalo dos valores (m = max - min + 1) é muito maior que a quantidade de números (m > 4n + 2^20), o programa usa automaticamente o **Radix Sort** (base 256) como reserva: <br />

      Temporal: O(n); <br />
      Espaço: O(n), independente do intervalo dos valores; <br />


============== INSTRUÇÕES DE COMPILAÇÃO ================== <br />

A lista à ser ordenada deve ser um .txt no mesmo diretório onde o executável está, com um número inteiro por linha. <br />
Compilar -> gcc -O2 -o counting_sort Sorting.c <br />
Executar -> ./counting_sort <br />

Opcional, informando os arquivos de entrada e saída: <br />
Executar -> ./counting_sort entrada.txt saida.txt <br />

Sem argumentos, a entrada é listaOdenada.txt e a saída é listaOrdenada_saida.txt. <br />

Se o arquivo tiver um valor que não seja inteiro ou que não caiba em int, o programa informa o arquivo e a linha do erro e termina com código de saída 1. <br />

======================================================= <br />


================= Tempo de Execução ================== <br />

<img src="./image.png" width="500"> <br />

================================================ <br />

<img src="https://i.programmerhumor.io/2026/03/fa2a91c9c05e834fb87afef3f1f919f52154ec2a776865a88837832a5883f997.jpeg" width="500">
