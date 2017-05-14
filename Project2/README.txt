T2G04
-----
Bárbara Sofia Lopez de Carvalho Ferreira da Silva
Igor Bernardo Amorim Silveira
Julieta Pintado Jorge Frade
----
No nosso programa várias threads modificam a variavel que serve de contagem do numero de pessoas na sauna, existindo assim uma secção critica. Deste modo, e para que nao exista conflito na modificação do valor da variável,é utilizado um mutex.

Quando a sauna está cheia mas os pedidos são do mesmo género das pessoas na sauna, esses pedidos ficam à espera que alguem saia para depois entrar. Para controlar esta situaçao foi usado um semáforo, que coloca a thread em espera caso a sauna esteja cheia.
