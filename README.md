# ComputacaoGrafica
Codigos desenvolvidos para computação gráfica, primariamente em C++

# REGISTRO DE IDEIAS
Esse texto não cumpre bem o papel de explicar o projeto, é apenas uma coleção de apontamentos. Detalhes de cada entrega estão no final do arquivo.

### Ideia inicial
Vamos implementar um modelo MVC (Model View Controller) para tentar deixar o projeto 
Model (src/core): Responsável pela descrição dos objetos, aqui definiremos o que é um ponto, reta, vetores, matrizes e afins, e suas operações. O model não sabe nada sobre o view ou o controller. 

View (src/graphics + src/window): Responsável pela renderização dos objetos (funções de desenho), pela lista de objetos a serem desenhados e pela conversão da window para o viewport. Terá conhecimento sobre os objetos do Model, mas não sobre o controller.

Controller (src/gui): Interface com a qual iremos interagir e visualizaremos o viewport, responsável pelos botões de interação e eventualmente atalhos. Ela precisará ter conhecimento do model e do view, para poder solicitar a criação dos objetos, sua renderização e manipulações da window.

tudo que está aqui pode e provavelmente vai ser mudado conforme aprimorarmos nosso intelecto, mas acho que ta uma boa divisão inicial


### EDIT ENTREGA 1:
No momento o projeto inclui algumas funcionalidades a mais. O código referente a entrega um se encontra em:
Panning/zooming: está localizado em `src/window/Window.hpp` e em `src/gui/Viewport.hpp`;
Display File: `src/window/DisplayFile.hpp`, os objetos estão definidos em `src/core`, optamos por uma mistura de OOP com Data Oriented Programming (Visando ter um código que seja eficiente em detrimento da praticidade da orientação a objetos).
Transformada de Viewport: Contém os métodos auxiliares em `src/window/Window.hpp`, que são utilizados pelo `src/graphics/Renderer.hpp` nos métodos de draw.
