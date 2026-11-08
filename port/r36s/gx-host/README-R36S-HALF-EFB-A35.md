# Resident Evil 4 R36S — reduzir EFB pela metade + compilar para RK3326

## Evidência do teste anterior

No último relatório `diagnostico_ps2_ultra_seguro_darkos.txt`, o
**Ultra Seguro** concluiu 180 segundos normalmente com:

- 1.256 quadros gráficos recebidos e apresentados;
- 255.299 desenhos e 46.556.174 triângulos;
- 103 programas gráficos, 54 hits do cache;
- **0 erros shaders, 0 erros GL, 0 erros IPC, saída 0**;
- zero desenhos excluídos por distância e 410 DOFs ignorados.
- taxa de aproximadamente **4,1–5,5 FPS** na primeira área mais pesada,
  com ~430–530 draws por frame; menus perto de 9 FPS.

**O número de triângulos e draws continua alto.** Otimização por
resolução só reduz trabalho por pixel, não substitui o decoder GX,
não simplifica malhas nem reduz draws.

## O que este experimento entrega (sem arriscar a versão estável)

Compilamos dois executáveis **separados** com o MESMO preset do Ultra
Seguro (2 luzes dinâmicas por canal, DOF off, neblina off, culling off)
e shader cache persistente:

1. `r36s_game_a35_full`: EFB original **640 × 528**, CPU RK3326
   otimizada.
2. `r36s_game_a35_half`: EFB **320 × 264** (metade de largura e
   altura, 1/4 dos pixels), mesmas flags Cortex-A35.

A dimensão interna da EFB, XFB e cópias GX muda no frontend
`gx_gl.cpp`; o jogo continua enviando coordenadas GX em 640 × 528.
O SDL2/KMSDRM amplia o framebuffer resultante para preencher a tela.
Não é apenas redução da resolução da janela SDL: os buffers de cor e
profundidade são alocados realmente com tamanho menor.

A redução para 320×264 pode causar **serrilhado, sombras imprecisas,
HUD desfocado, diferenças em efeitos que leem EFB e possível
incompatibilidade nas cópias EFB**. É experimental, e o usuário deve
voltar ao executável estável se vir elementos pretos, texturas
ausentes ou frames quebrados. O DOF segue desligado, pois essa
opção já havia sido testada com sucesso.

## Otimizações de compilação

Para os dois executáveis R36S exclusivos:

- `-O3`: otimização mais agressiva de loops e código C++;
- `-mcpu=cortex-a35 -mtune=cortex-a35`: instruções e agendamento
  específicos aos **4 Cortex-A35** do Rockchip RK3326;
- `-flto=auto`: otimização entre unidades de compilação e na linkedição;
- `-ffunction-sections -fdata-sections -Wl,--gc-sections`:
  remoção de funções/dados não usados.
- **Sem `-ffast-math`**: operações float do decoder e do
  estado gráfico precisam manter comportamento preciso;
  `-ffp-contract=off` e `-fno-omit-frame-pointer` preservados
  para compatibilidade e diagnósticos.

**Escopo real:** essas flags se aplicam ao frontend R36S e ao
renderizador GX, comunicação IPC e decodificação de texturas
ligados nesse executável. O `re4_coreproc` e as bibliotecas
`libre4_main.so`, `libre4_platform.so`, os 114 módulos e o
kernel do DarkOSRE **não são recompilados** e permanecem inalterados.
Uma recompilação do core para A35 deverá ser tratada separadamente.

## Por que não remover árvores automaticamente?

O pacote GX que chega em `GxRenderer::Draw` contém
`vertexOffset`, layout, índices, matrizes e estado GPU, **mas não
contém identificadores semânticos da malha** ("árvore",
"ganado", "planta", "Leon", "inimigo", "item"). As tentativas de
remover desenhos usando distância da câmera no antigo Ultra
deletaram **235.546 draw calls** e deixaram objetos pretos ou
ausentes. Não ativamos heurísticas que possam remover elementos
necessários ao jogo.

Para remover plantas/árvores com segurança, é preciso catalogar
os IDs dos arquivos e modelos do jogo na primeira fase e classificar
individualmente os não essenciais, ou gerar LODs a partir dos
objetos originais. Isso ainda não foi implementado nem prometido
por esse pacote.

## Instalação e comparação A/B

O artifact `CubeEvil4-R36S-Meia-Resolucao-A35-ARM64` contém:

- `cubeevil4_live_gx/r36s_game_a35_full`
- `cubeevil4_live_gx/r36s_game_a35_half`
- `RE4-R36S-A35-EFB-Teste.sh`
- `RE4-R36S-A35-Normal.sh`
- **`RE4-R36S-Metade-Resolucao.sh`**
- `LEIA-ME-R36S-MEIA-RESOLUCAO.md`

Extraia tudo em `/roms/ports` e **não apague**:

- `cubeevil4_live_gx/r36s_game_boot` e `RE4-R36S-Estavel.sh`;
- `cubeevil4_live_gx/re4_coreproc`, módulos e bibliotecas;
- os dados do jogo extraídos `cubeevil4_live_gx/data/`, incluindo
  `data/memcard/card-a.bin`.

Primeiro rode **`RE4-R36S-Metade-Resolucao.sh`**, entre na
primeira área e observe personagens, céu, árvores, fundo, HUD, armas,
sombra e itens. Tempo máximo 180 segundos, ESC ou SELECT+START
por 2 segundos para saída (se SELECT mapear para SDL BACK).

Seu relatório exclusivo será:

`/roms/ports/cubeevil4_live_gx/diagnostico_a35_efb_320x264_darkos.txt`

Depois rode `RE4-R36S-A35-Normal.sh` seguindo a mesma rota
e obtenha:

`/roms/ports/cubeevil4_live_gx/diagnostico_a35_efb_640x528_darkos.txt`

Envie **ambos** para comparar apenas o custo da resolução, pois as
duas versões já têm as mesmas flags Cortex-A35.

### Critérios

- `R36S_EFB_RENDER_SCALE=0.50` e `EFB_INTERNAL_SIZE=320x264`
  demonstram que há buffers realmente menores;
- `FPS_PRESENTED`, `DRAWS`, `TRIANGLES`, `CORE_PID_VmRSS`
  e `FRONTEND_VmRSS` em trechos semelhantes;
- `CRASH_STAGE`, `SHADER_ERRORS`, `GL_ERRORS` e `EXIT_CODE`
  para estabilidade;
- imagem visualmente aceitável é **obrigatória**: estatísticas GL sem
  erros não provam que o resultado gráfico esteja correto.

A otimização não foi validada no R36S físico. Se os dois quadros
médios forem quase iguais, o gargalo é CPU/submissão de draws/IPC,
não preenchimento de pixels. Não espere ganho de 4× FPS por
reduzir para 25% dos pixels.
