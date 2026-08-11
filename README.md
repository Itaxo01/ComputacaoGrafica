<div align="center">

# Sistema Gráfico Interativo

**Um sistema gráfico 2D/3D completo escrito do zero em C++ — sem OpenGL, sem bibliotecas de matemática, sem engine.**

Modelagem, transformações, projeção, perspectiva, recorte, rasterização, z-buffer e iluminação Phong rodando de forma paralelizada na CPU.

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://en.cppreference.com/)
[![Dear ImGui](https://img.shields.io/badge/UI-Dear%20ImGui-1f6feb?style=flat-square)](https://github.com/ocornut/imgui)
[![GLFW](https://img.shields.io/badge/Janela-GLFW-orange?style=flat-square)](https://www.glfw.org/)
[![Linux | Windows](https://img.shields.io/badge/Plataformas-Linux%20%7C%20Windows-lightgrey?style=flat-square)](#-compilação)
[![License](https://img.shields.io/badge/Licença-ver%20LICENSE-green?style=flat-square)](LICENSE)

<img src="media/demo.gif" alt="Demonstração do sistema gráfico" width="90%">

</div>

---

## Sobre o projeto

Este é um **sistema gráfico interativo** desenvolvido para a disciplina de Computação Gráfica. Implementamos a **versão extendida** do sistema gráfico, no qual também fazemos a **rasterização e iluminação**. O projeto foi feito em C++, contrário ao normal da disciplina, que é em python, para conseguir extrair o máximo do processador e tornar possível a renderização sem o uso de CUDA ou alguma biblioteca gráfica como o OpenGL. 

O OpenGL é utilizado indiretamente, pois a biblioteca de interface gráfica **Dear ImGui** utiliza o OpenGL em sua renderização. Porém, em nosso código ele é usado apenas para **exibir uma textura na tela**. A pipeline completa, importação/criação dos objetos, **transformações do objeto**, **normalização das coordenadas**, **rotação da window**, **transformações de perspectiva**, **clipping das bordas**, **z-buffer para checagem de profundidade**, **iluminação Phong**, **transformação de viewport** e **rasterização** de cada pixel em um **FrameBuffer** com SSAA, é toda feita via código e executada em CPU. No final de cada ciclo de renderização, apenas copiamos a textura do FrameBuffer gerado para que o OpenGL mostre na tela.    

- **Nenhuma biblioteca de álgebra linear**: `mat4`, `Point` e as transformações homogêneas são próprias.
- **Nenhuma rasterização de GPU**: os triângulos, linhas e pontos são preenchidos em um framebuffer
  na CPU (função de aresta / baricêntricas, Bresenham)
- **Nenhum shader**: a iluminação Phong é avaliada em C++, por face, por vértice ou por pixel, de acordo com o seletor na interface (None - Flat - Gourand - Phong).

O resultado é um "mini renderizador" navegável, similar ao Blender em certos aspectos.

### A pipeline, do objeto ao pixel

```mermaid
flowchart LR
    subgraph S1["1 · O objeto nasce"]
        direction TB
        U1["<b>Desenhado na interface</b><br/>clique no viewport ou<br/>coordenadas digitadas"]
        U2["<b>Importado de arquivo</b><br/>.obj + .mtl"]
        FAC["<b>A factory monta a malha</b><br/>curvas e superfícies são<br/>tesseladas aqui — uma vez só"]
        EM["<b>O gerenciador registra</b><br/>id, nome e metadados"]
        DF["<b>Display File</b><br/>a cena que o renderizador lê"]
        TRF["<b>Transformar ou animar</b><br/>muda a matriz do objeto<br/>e invalida o cache"]
        U1 --> FAC
        U2 --> FAC
        FAC --> EM --> DF
        TRF -.-> DF
    end

    subgraph S2["2 · Da cena para a tela <i>(refeito só quando algo muda)</i>"]
        direction TB
        P{"Com<br/>perspectiva?"}
        N["<b>Direto para as coordenadas normalizadas</b><br/>uma matriz só carrega objeto e window;<br/>a projeção paralela apenas descarta o Z"]
        V["<b>Primeiro para o espaço da câmera</b><br/>a divisão perspectiva ainda não pode acontecer"]
        CN["<b>Corta o que está atrás do olho</b><br/>antes da divisão por w"]
        PR["<b>Aí sim, projeta</b><br/>x' = x·d / (z+d)"]
        CL["<b>Recorta nas bordas da window</b><br/>Liang-Barsky ou Cohen-Sutherland;<br/>polígonos por Sutherland-Hodgman"]
        TRI["<b>Junta os triângulos preenchidos</b><br/>descarta os de costas e ordena<br/>por profundidade"]
        VP["<b>Espalha na área do viewport</b><br/>as normalizadas viram pixels, × SSAA"]
        P -- não --> N
        P -- sim --> V --> CN --> PR
        N --> CL
        PR --> CL
        CL --> TRI --> VP
    end

    subgraph S3["3 · Virando pixel <i>(todo frame)</i>"]
        direction TB
        BG["<b>Eixos e grid</b><br/>vão direto para o ImGui,<br/>por baixo de tudo"]
        LI["<b>As luzes são refeitas agora</b><br/>fora do cache, para acenderem na hora"]
        RA["<b>Rasteriza</b><br/>uma faixa de linhas por thread;<br/>em cada pixel, a profundidade e o Phong"]
        RS["<b>Reduz o supersampling</b><br/>a média dos sub-pixels é o anti-aliasing"]
        PRE["<b>Entrega a imagem</b><br/>o framebuffer vira textura —<br/>o único instante com OpenGL"]
        LI --> RA --> RS --> PRE
        BG -.-> PRE
    end

    DF --> P
    VP --> RA

    classDef entrada fill:#8957e5,stroke:#6e40c9,color:#fff
    classDef geom    fill:#1f6feb,stroke:#1158c7,color:#fff
    classDef corte   fill:#bf8700,stroke:#9e6a03,color:#fff
    classDef raster  fill:#238636,stroke:#1a7f37,color:#fff
    classDef saida   fill:#da3633,stroke:#b62324,color:#fff
    classDef decisao fill:#30363d,stroke:#8b949e,color:#fff
    classDef cena    fill:#0d7d8c,stroke:#0a626e,color:#fff
    classDef etapa   fill:none,stroke:#8b949e,stroke-dasharray:4 4,color:#8b949e

    class S1,S2,S3 etapa
    class U1,U2,FAC,EM,TRF entrada
    class DF cena
    class P decisao
    class N,V,PR,TRI,VP geom
    class CN,CL corte
    class BG,LI,RA,RS raster
    class PRE saida
```

O corte entre as etapas 2 e 3 é o que segura o desempenho: **a etapa 2 inteira é cacheada** e só roda de novo
quando a cena, a câmera ou o tamanho do canvas mudam — arrastar uma luz não recalcula geometria nenhuma.
A etapa 3 roda sempre.

Vale notar também que, com o shading ligado, a transformação da etapa 2 guarda de lado as posições de
mundo dos vértices e as normais suaves (média das faces vizinhas): a projeção destrói essa informação,
mas o Phong precisa dela lá no fim, por pixel.

---

## O que o aplicativo faz

### Modelagem 2D
Como um dos requisitos da matéria, as versões iniciais do **SGI** são 2D. São exclusivos do modelo 2D:

| Recurso | Detalhe |
|---|---|
| **Primitivas** | Ponto, linha, wireframe (polilinha) e polígono (preenchido ou não) |
| **Curvas de Bézier** | Cadeia de segmentos cúbicos com continuidade nas âncoras (`A C C A C C A…`). Possui continuidade C0 |
| **Curvas B-Spline** | Avaliação por **forward differences**, a partir de N pontos de controle. Possui continuidade C2 |
| **Criação por clique** | Cliques no viewport, `Enter`/duplo-clique confirma, `Esc` cancela |


### Modelagem 3D
Posteriormente, o **SGI** foi extendido para suportar 3D, e assim surgem as principais implementações. É exclusivo do modo 3D:

| Recurso | Detalhe |
|---|---|
| **Câmera VRC** | Sistema de referência de vista completo (VRP, VPN, VUP) com órbita, pan e zoom |
| **Projeção paralela** | Ortográfica, obtida alinhando a VPN ao eixo +Z |
| **Projeção perspectiva** | COP a uma distância focal ajustável — do "telefoto" à grande-angular |
| **Z-Buffer** | Z-buffer para renderização de objetos com diferentes profundidades no mesmo pixel, onde o mais próximo é renderizado |
| **Primitivas** | Primitivas adaptadas ao 3D, ponto 3D, linha, wireframe, poligono... Criação dos objetos com o mouse é desabilitada no modo 3D. |
| **Superfícies bicúbicas** | Patches de 16 pontos de controle, em versão **Bézier** e **B-Spline** |
| **Bounding box** | Caixa envolvente opcional para dar noção de profundidade |

### Renderização
Depois que os valores do objeto são definidos no `src/gui/ObjectCreator.hpp` ou pelo `src/io/ObjSerializer.hpp`, ele passa para o `src/core/EntityManager.hpp`, onde sua criação é efetivada e o objeto é guardado no `src/window/DisplayFile.hpp`. A cada frame, o loop renderiza os objetos do display file no viewport, executando o pipeline:

| Recurso | Detalhe |
|---|---|
| **Rasterizador em CPU** | Framebuffer próprio, os objetos são desenhados utilizando métodos de desenhar ponto, linha e triângulo preenchido. Paralelizado, cada thread recebe uma faixa de y para renderizar (Varre todos os objetos), não há condição de corrida. |
| **Recorte 2D** | **Liang-Barsky** e **Cohen-Sutherland**, alternáveis em tempo real (Requisito da disciplina); polígonos por Sutherland-Hodgman |
| **Recorte 3D** | O recorte 3D é o mesmo do 2D, depois de aplicada a projeção. No 3D, por conta da perspectiva, também é feito o clipping no plano *near* antes da divisão perspectiva |
| **Visibilidade** | **Z-buffer** por pixel *ou* **algoritmo do pintor** (ordenação por profundidade) |
| **Back-face culling** | Com convenção de winding invertível para modelos "ao contrário" |
| **Anti-aliasing** | **SSAA** de 1× a 4×: rasteriza k^2 framebuffers para obter uma resolução maior e faz resolve box na apresentação. Isso deixa as linhas mais suaves, porém aumenta o custo computacional drasticamente |
| **Paralelismo** | Quase todo o pipeline é paralelizado via criação de threads localmente em cada etapa, com implementação nativa porém também com a biblioteca TBB (Utilizada inicialmente por ser melhor, com certas otimizações se tornou um pouco obsoleta). |

### Iluminação
A iluminação, como mencionada anteriormente, implementa o modelo de Phong, que divide a luz entre luz Ambiente, Difusa e Especular. Esses parâmetros são editáveis apenas no arquivo .mtl que é carregado junto ao arquivo .obj de mesmo nome. 

| Recurso | Detalhe |
|---|---|
| **Modelo de Phong** | Ambiente + difuso (Lambert) + especular, com `ka/kd/ks/Ns` vindos do `.mtl`. Sob um ponto P, calcula qual deveria ser sua cor. Esse é o unico modelo de iluminação implementado, os demais variam de acordo com o escopo em que são usados |
| **Flat shading** | Calcula o Phong uma só vez por face, a face inteira recebe a mesma cor |
| **Gouraud shading** | Calcula o Phong para cada vértice do triângulo, cada pixel da face recebe uma interpolação com peso das cores dos vértices |
| **Phong shading** | Calcula o Phong para **cada pixel** rasterizado. É o mais pesado |
| **Luz ambiente** | Podemos mudar a cor da luz ambiente, apesar de sua intensidade ser definida pelo .mtl |
| **Luzes** | É possível adicionar quantos pontos de luz você quiser, com posição, cor e intensidade editáveis |
| **Headlight** | Luz farol para visualização dos objetos |

### Transformações e animação

- Translação, escala e rotação **em torno do centro do objeto, da origem ou de um ponto arbitrário**.
- Rotação 3D em torno de um **eixo qualquer**.
- As transformações são **acumuladas em um buffer** e aplicadas de uma vez como uma única matriz
  composta (`Apply all transformations`), requisito da disciplina.
- **Múltiplos objetos** podem ser transformados simultaneamente.
- **Animação**: qualquer sequência de transformações pode ser interpolada ao longo de N segundos, opcionalmente em *loop* — inclusive lida de um script (veja `models/donut_spin.txt`, que implementa o clássico 'Donut Spin').

### Importação e exportação

- **`.obj` + `.mtl`** completos: vértices, faces, grupos, materiais, `mtllib`/`usemtl`.
- Importar `"models/mk4"` carrega automaticamente o `.obj` e o `.mtl` correspondente.
- Exportação da cena inteira de volta para `.obj`/`.mtl`.
- A pasta [`models/`](models/) já vem com vários modelos de teste (`mk4`, `cristo`, `Goro`,
  `subzero`, `Donut`, superfícies de Bézier e B-Spline…).

---

## Galeria

<div align="center">

| <img src="media/curvas.gif" width="100%"> | <img src="media/superficies.gif" width="100%"> | <img src="media/perspectiva.gif" width="100%"> |
|:--:|:--:|:--:|
| **Curvas 2D**<br><sub>Bézier e B-Spline construídas clique a clique,<br>recortadas nas bordas da window</sub> | **Superfícies bicúbicas**<br><sub>patches de Bézier e B-Spline tesselados,<br>preenchidos e com z-buffer</sub> | **Câmera em perspectiva**<br><sub>órbita e distância focal em uma cena<br>com dezenas de malhas importadas</sub> |

</div>

<br>

<div align="center">
<img src="media/shaders.gif" width="100%">

**Modelos de iluminação** — alternando entre *None*, *Flat*, *Gouraud* e *Phong*.
</div>

<br>

<div align="center">
<img src="media/animacoes.gif" width="100%">

**Animação por script** — o donut de `donut.c` girando a partir de `models/donut_spin.txt`, com transformações interpoladas em loop. Modelo exportado do Blender encontrado online e exportado como .obj. O modelo do Donut é bem pesado, cerca de **300 mil faces**, para renderizá-lo foi utilizada a versão em **CUDA**, que **não** foi finalizada (Ver branch no Github).

</div>

---

## Controles

| Ação | Atalho |
|---|---|
| Zoom | Scroll do mouse ou `Ctrl` + `↑`/`↓` |
| Pan / translação da window | Arrastar com o **botão direito** ou `Shift` + setas ou `Shift` + Scroll do mouse |
| Rotação da window (2D e 3D) | `Ctrl` + `Shift` + `←`/`→` ou `Ctrl` + `Shift` + Scroll (Funciona melhor com scroll analógico ou touchpad)|
| Distância focal (perspectiva) | `Shift` + scroll |
| Confirmar criação | `Enter` ou duplo-clique |
| Cancelar criação | `Esc` |

### Janelas da interface


- **Viewport** — É onde o framebuffer é desenhado, e é onde podemos interagir com o mundo e modificar as configurações de renderização.
- **Object Creator** — menu de criação de objetos, e também o IO de arquivos .obj (e .mtl, se presentes).
- **Object Manager** — lista de objetos, detalhes, seleção múltipla e manipulação dos objetos.
- **Lighting** — modelo de shading, luz ambiente, headlight e editor de luzes pontuais.
- **Log** — registro das ações, ignorável.

Todas as janelas são móveis e redimensionáveis; `Reset Layout` devolve tudo ao lugar
(o layout padrão é calculado em função da resolução e do DPI do monitor).

---

## Compilação

**Dependências:** compilador com C++20, GLFW e (opcional, não é necessário) Intel TBB.

```bash
# Ubuntu / Debian
sudo apt install build-essential libglfw3-dev libtbb-dev

make fast -j16     # build otimizado (-O3)
./programa_foda.out
```

O `make` detecta o TBB automaticamente: se estiver disponível, a paralelização usa
`std::execution`; caso contrário, cai em um fallback nativo — o programa funciona igual,
só mais devagar.

<details>
<summary><b>Cross-compilação para Windows a partir do Linux</b></summary>

```bash
make windows -j16        # standalone, sem TBB (mais lento, mas um .exe só)
make windows_fast -j16   # com TBB, empacota as DLLs necessárias em um .zip
```

Usa o MinGW-w64 e os binários pré-compilados em `libs/windows/`. O build `windows_fast`
exige que `libtbb12.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll` e `libwinpthread-1.dll`
fiquem no mesmo diretório do executável (o alvo já cuida disso).

</details>

---

## Bugs e algumas considerações

Existem alguns bugs misteriosos que não consegui reproduzir consistentemente, especialmente relacionados a exclusão de objetos, que tende a crashar o programa esporadicamente. 

O programa também pode requerir um computador relativamente bom para reproduzir os resultados gravados. 
Ele conhecidamente não está no seu auge, já que apesar de fazer o processamento em paralelo, não aproveita a GPU (Com excessão da branch experimental em CUDA) e não aproveita instruções avançadas de vetorização da CPU, ao menos não explicitamente (Pode ser que o compilador otimize algo do tipo).
Periodicamente eu tento melhorar um pouco esse desempenho.
Caso encontre algum bug ou queira sugerir alguma melhoria, mande uma mensagem para <kauan.fank@gmail.com>


## Autores

Desenvolvido por **[Kauan Fank](https://github.com/Itaxo01)** e **[Abel Scheidt](https://github.com/abel-scheidt)**
para a disciplina de Computação Gráfica.

Interface construída com [Dear ImGui](https://github.com/ocornut/imgui) e [GLFW](https://www.glfw.org/).
