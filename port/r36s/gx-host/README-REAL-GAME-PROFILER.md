# CubeEvil4 R36S — investigação de desempenho da GPU

## Evidência no R36S real

`RE4-R36S-Estavel.sh` (modo 5) passou três minutos de execução com
Leon dentro do jogo, sem falhas gráficas ou de IPC:

- 1.415 quadros gráficos apresentados e 0 erros GL, shader ou IPC
- 15.516.500 triângulos, 156.290 chamadas de desenho, 126 shaders
- Imagem visível no driver Mali-G31 GLES3.2, com `glFinish()` antes do swap
- RSS final ~165 MiB no frontend e ~123 MiB no núcleo; **VmSize não é RAM real**
- A primeira sala chegou a `player free`, com controles respondendo
- O jogo **ainda não tem saída de áudio SDL2**; amostras só são descartadas

**O obstáculo atual é desempenho, não correção visual:** a cena inicial
ficou perto de 9 FPS e a primeira sala caiu para cerca de 3–4 FPS,
com mais de 500 draws por quadro e vários megabytes de geometria.

## Novo diagnóstico

Este pacote compila o mesmo frontend GX em um binário separado
`r36s_game_profile`, habilitando **apenas cronômetros por etapa**.
Ele **não substitui** `r36s_game_boot` nem muda as flags GL funcionais.
Registra uma amostra a cada cinco segundos:

`STAGE_TIMING_MS SAMPLES=... GX_RUN_AVG=... GX_RUN_MAX=...
PRESENT_AVG=... GL_FINISH_AVG=... EGL_SWAP_AVG=...`

As quatro etapas medidas são:

1. **GX_RUN:** interpretação dos comandos GX e submissão OpenGL;
2. **PRESENT:** cópia do framebuffer GX para a tela (sem swap);
3. **GL_FINISH:** espera de conclusão de comandos GPU;
4. **EGL_SWAP:** troca real dos buffers de apresentação.

Os tempos são de **relógio de parede das chamadas no processo**, não
timings de hardware da GPU; por exemplo, `glFinish()` pode aguardar
operações previamente enfileiradas. As métricas incluem espera pelo
driver e não permitem atribuir custo a GPU ou CPU de forma definitiva.

## Instalação

Baixe `CubeEvil4-R36S-Perfil-GPU-ARM64` na ação
`R36S - profile real game GPU frame stages`, extraia em
`/roms/ports` preservando:

- `cubeevil4_live_gx/r36s_game_boot` (a versão estável);
- `cubeevil4_live_gx/re4_coreproc`, bibliotecas e `modules`;
- `cubeevil4_live_gx/data` e memory card `card-a.bin`.

Execute pelo menu Ports **`RE4-R36S-Perfil-GPU.sh`**.
O teste roda por até 180 segundos (3 minutos), inclusive para atingir
a área jogável; para sair antes, segure **SELECT + START por 2 segundos**
ou use ESC se houver teclado. SELECT deve mapear para SDL BACK.
Faça backup de `data/memcard/card-a.bin` se necessário.

O arquivo de relatório **independente** é:

`/roms/ports/cubeevil4_live_gx/diagnostico_perfil_gpu_darkos.txt`

Pode mandar apenas esse TXT. **Não envie ISO ou arquivos do jogo**.

## Próximas otimizações

Vamos identificar a etapa dominante antes de testar alterações,
pois removendo `glFinish` a versão anterior caía no frame 16.
Se `GX_RUN` dominar, investigar batch/VAOs e uploads de buffer;
se `GL_FINISH` dominar, verificar custo GPU de shaders, texturas,
EFB/XFB e pontos de sincronização; se `EGL_SWAP` dominar, estudar
caminho KMSDRM/Swap interval e buffers.
