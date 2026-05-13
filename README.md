# ComputacaoGrafica
Codigos desenvolvidos para computação gráfica, primariamente em C++

# INSTRUÇÕES DE USO DA INTERFACE
A interface contém atualmente 4 componentes:

* **Viewport:** 
    * Canvas onde os objetos são desenhados e visualizados.
    * Podemos interagir com o viewport de diversas formas:
        * **Zoom** -> Scroll do mouse ou ctrl + Up/Down arrow.
        * **Translação** -> Clicar com o botão direito do mouse e arrastar ou shift + arrow key
        * **Rotação da Window** -> Ctrl+Shift+Right/Left arrow key.
        * **Criação de objetos:**
            * Habilitar a criação no menu de Criação de objetos
            * Selecionar o tipo do objeto
            * Dar um nome (opcional, gerado nome automático com o ID (O nome não é usado como ID))gi
            * Selecionar a cor do objeto (Opcional, default branco)
            * Cada objeto tem sua forma de criar clicando no viewport:
                * ponto é apenas um click;
                * linha são dois clicks;
                * wireframe são n clicks, double click efetua a criação.
                * Poligono são n clicks, double click efetua a criação (Primeiro ponto é duplicado e o último se conecta a este). É possível selecionar se o polígono é preenchido ou não
                * Curva 2D existem dois métodos de criação: Bézier e BSplines. Para ambos é possível selecionar a quantidade de steps da continuidade.
                    - **Bezier**: Para bezier são n clicks, cada ponto múltiplo de 3 é uma âncora e os demais são pontos de controle (0A, 1C, 2C, 3A, 4C, 5C, 6A...). É necessário terminar em um ponto âncora para criar o objeto.
                    - **BSplines**: Para BSplines são n clicks, cada ponto é um ponto de controle. É necessário ter pelo menos 4 pontos para criar o objeto.
            * Para cada um dos objetos é possível cancelar sua criação apertando esc, ou confirmar a criação apertando enter ou double click (só faz sentido para aqueles que possuem n pontos).
    * Também é possível habilitar e desabilitar a renderização da grid do viewport.

* **Menu de Criação de Objetos:**
    * Menu básico para criação de objetos. Desabilitar a criação faz com que ao clicar no viewport não se crie objetos.
    * Importação de arquivos:
        * No momento, a importação e exportação de arquivos não segue o .obj, e é feita de forma customizada (Será modificado em futuras entregas)

* **Object Manager:**
    * Mostra a lista de objetos atuais.
    * Selecionar um objeto permite ver seus detalhes e efetuar as operações de transformação (Detalhes e transformações são duas abas dessa janela).
    * É possível selecionar múltiplos objetos e realizar operações com multiplos objetos simultaneamente.
    * As transformações são guardadas em um buffer a medida que são adicionadas (Cumprindo o requisito da entrega de acumular a matriz de transformação), e para aplicá-las é necessário selecionar "Apply all transformations" no final da janela da aba de transformações. 
    * Clicar com o direito em um objeto abre um menu de operações (No momento as transformações não estão no menu, apenas a deleção. As transformações estão, entretanto, implementadas na aba de transformações).

* **Log:**
    * Apenas um log de registro de algumas ações na interface. Útil para o desenvolvimento. Possui alguns erros não relevantes para a implementação.

As janelas podem ser reposicionadas e remodeladas a vontade (A depender da resolução do monitor, é possível que alguma janela não inicialize corretamente).

# COMPILAÇÃO
Utilizar o makefile.
Recomendo ter a biblioteca TBB da intel instalada, mas não é necessário, apenas deixa o programa mais rápido.

# COMPILAÇÃO PARA WINDOWS
O make tem uma rotina para compilar o .exe do windows no linux. Para isso, ele utiliza a pasta libs/, que possui os binários pré compilados para windows.
Como estamos utilizando o TBB, o .exe precisa ter o arquivo libtbb12.dll junto dele no mesmo diretório para funcionar (e outras).
Não recomendo utilizar isso, pois involve outras dll's que precisam estar compiladas sob o mesmo compilador.
Na entrega do trabalho, já disponibilizamos nossa versão compilada para windows.