T2G04
-----
B�rbara Sofia Lopez de Carvalho Ferreira da Silva
Igor Bernardo Amorim Silveira
Julieta Pintado Jorge Frade
----
No nosso programa v�rias threads modificam a variavel que serve de contagem do numero de pessoas na sauna, existindo assim uma sec��o critica. Deste modo, e para que nao exista conflito na modifica��o do valor da vari�vel,� utilizado um mutex.

Quando a sauna est� cheia mas os pedidos s�o do mesmo g�nero das pessoas na sauna, esses pedidos ficam � espera que alguem saia para depois entrar. Para controlar esta situa�ao foi usado um sem�foro, que coloca a thread em espera caso a sauna esteja cheia.
