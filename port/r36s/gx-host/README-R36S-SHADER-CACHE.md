# Resident Evil 4 GameCube no R36S — cache de shaders GLES, sem reduzir qualidade

## Por que testar

O R36S/ DarkOSRE detectou `Mali-G31`, `OpenGL ES 3.2`, 895 MiB
gerenciados pelo Linux, sem swap. Na última sessão de 180s
(`diagnostico_gx_sem_probes_darkos.txt`), o jogo executou
1.129 quadros, 218.001 draws, 126 shaders, 0 erros GL/IPC/shaders,
código 0. Em partes pesadas da cena o jogo segue em ~3,8–4,2 FPS.

A versão **sem consultas de GPU** não mostrou melhora consistente sobre
o perfil anterior. O cache de shader não é uma solução de 4 para 30 FPS,
mas reduz compilação na entrada de áreas e poderá diminuir paradas
de vários segundos quando um shader novo aparece.

## Infraestrutura JÁ existente no renderizador GX

O código em `gx_gl.cpp` já implementa `SetProgramCache()` e usa:

- `GL_PROGRAM_BINARY_RETRIEVABLE_HINT`, `glGetProgramBinary` na gravação;
- `glProgramBinary`, verificando `GL_LINK_STATUS`, na leitura;
- chave calculada a partir do texto vertex+fragment e identificação
  do driver (`GL_RENDERER|GL_VERSION`);
- queda automática para compilação normal se um binário for inválido.

Essa implementação estava desconectada do host GX do R36S.
O executável experimental `r36s_game_shadercache` ativa o cache antes do
primeiro quadro real, mantendo VSync1 + `glFinish` antes do swap.

**Compatibilidade:** se a biblioteca GLES da Mali-G31 informar
`GL_NUM_PROGRAM_BINARY_FORMATS=0`, o cache fica desativado e o teste
continua por compilação normal. Binários de GPUs/versões de driver
diferentes não são portáteis. O cache fica no seu cartão, não na RAM.

## Instalação sem modificar o seu Resident Evil 4 estável

O artifact `CubeEvil4-R36S-Shader-Binary-Cache-ARM64` contém apenas:

- `cubeevil4_live_gx/r36s_game_shadercache` (novo binário isolado)
- `RE4-R36S-Cache-Shaders.sh` (novo launcher de Ports)
- `LEIA-ME-SHADER-CACHE.md` (instruções)

Extraia em `/roms/ports`. **Não apague nem substitua**
`r36s_game_boot`, `RE4-R36S-Estavel.sh`,
`re4_coreproc`, `modules/`, bibliotecas nem
`data/` e o memory card virtual.

## Teste A/B correto

1. Execute `RE4-R36S-Cache-Shaders.sh`, carregue a primeira área e
   deixe rodar os três minutos completos. Essa execução deve preencher
   o cache.
2. Feche naturalmente pelo tempo ou com SELECT+START por 2 segundos
   (se SELECT mapear para SDL BACK).
3. **Execute o mesmo launcher uma segunda vez**, siga o mesmo trajeto e
   anote se a entrada na área ficou menos travada. A segunda execução
   deve registrar `SHADER_CACHE_FINAL_HITS>0` caso o driver suporte
   binários e o diretório esteja gravável.

Logs **diferentes para cada execução**:

- `cubeevil4_live_gx/diagnostico_shader_cache_teste1_darkos.txt`
- `cubeevil4_live_gx/diagnostico_shader_cache_teste2_darkos.txt`

Verifique os campos:
`SHADER_CACHE_BINARY_FORMATS`, `SHADER_CACHE_ENABLED`,
`SHADER_CACHE_FINAL_PROGRAMS`, `SHADER_CACHE_FINAL_HITS`,
`SHADER_CACHE_FINAL_MISSES_OR_UNSUPPORTED`, `SHADER_CACHE_FILES_AFTER`,
`FPS_PRESENTED`, `EXIT_CODE`.

Cache persistente:
`/roms/ports/cubeevil4_live_gx/cache/shader_binaries/`
(no cartão SD, sem descarregar dados originais).

## RAM disponível e por que cache não aumenta FPS sozinho

- 895 MiB físicos gerenciados pelo sistema (console anunciado como 1 GB).
- Nas cenas medidas, RSS do frontend ~165–173 MiB e RSS do core
  ~115 MiB, mas são **dois processos**, com memória compartilhada;
  RSS não mede toda a RAM usada pela GPU nem deve ser somada sem cuidado.
- A área de 500+ draws/quadro gasta ~160–190 ms em
  `renderer.Run` após os shaders já terem sido gerados: carregar
  binários antigos não elimina os custos de draw, estado e GPU.
- O renderizador também **já tem** cache de shaders em memória durante
  cada execução, uma `uniform stream` de 16 MiB e cache de texturas.

Em trabalhos futuros, reservar por exemplo **32–64 MiB** para
cache LRU de dados descompactados (não de frames) poderia reduzir
trabalho redundante, mas só após medir acertos e falhas. Uma GPU Mali-G31
e a CPU compartilham a RAM física. Não é seguro destinar tudo que o
`free` informa como disponível para o jogo. Com zero swap, esgotar
memória pode matar o processo e corromper a sessão de save.

## Compilação assíncrona como Cemu

Cemu pode usar compilação assíncrona com Vulkan, pulando
temporariamente shaders que ainda não estão prontos. No OpenGL ES
atual seria necessário um contexto EGL compartilhado em outra thread,
sinalização de conclusão e políticas para não desenhar com shader
incompleto. Isso tem riscos de artefatos no GameCube e complicações com
o driver Mali e o KMSDRM. **Não foi implementado neste patch.**

Primeiro confirmar cache de binários on-disk de modo síncrono, depois
avaliar pré-compilação de shaders conhecidos com contexto separado.
A ordem correta depende das estatísticas de `SHADER_CACHE_FINAL_HITS`.

## Limitações

Não há áudio SDL2 nesta versão, e o cache não promete aumentar FPS
durante gameplay contínuo. Uma compilação de CI bem-sucedida demonstra
apenas build ARM64, não funcionamento de binários do driver no console.
O teste deve ser feito no R36S físico, preservando os saves.
