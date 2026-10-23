# CubeEvil4 R36S / DarkOSRE — gráficos Low Spec inspirados no RE4 do PS2

## Objetivo

O console **já executa gameplay real** sem falhas visuais conhecidas,
mas a primeira área pesada apresenta aproximadamente **3,6–4,2 quadros
processados por segundo**. Reduzir efeitos e distância de desenho pode
ajudar sem depender exclusivamente de cache de shaders. Este é um
**experimento de desempenho, NÃO uma versão otimizada validada**.

O port original para PlayStation 2 sacrificou resolução e detalhes dos
modelos, efeitos de luz e neblina, reflexos/água e distância visível.
**Aqui não possuímos os modelos LOD nem o código de cena PS2**, portanto
não aplicamos automaticamente a mesma geometria; simplificamos os
caminhos gráficos GX disponíveis nesta implementação.

## Perfil gráfico (selecionável no menu Ports)

| Script | Luzes dinâmicas por canal | DOF/desfoque | Fog/neblina | Cortar objetos distantes |
|---|---:|---|---|---|
| `RE4-R36S-PS2-Balanceado.sh` | até 2 | desativado | mantida | NÃO |
| `RE4-R36S-PS2-Leve.sh` | até 1 | desativado | mantida | atrás do limite de 90 unidades de câmera |
| `RE4-R36S-PS2-Ultra.sh` | 0 (ambiente apenas) | desativado | removida | atrás do limite de 45 unidades de câmera |
| `RE4-R36S-Estavel.sh` | original | original | original | NÃO (versão anterior preservada) |

**A distância é experimental**: não há garantia de que 90/45 unidades
equivalem a metros do jogo. O teste usa coordenadas de câmera do GX,
não distância de simulação de inimigos. Corta *somente um draw call*
se todos os vértices de uma malha forem mais distantes que o limite;
não mexe em física, IA nem colisões. Para preservar HUD e personagens,
o culling ignora projeções ortográficas, draw calls indexados por
matriz por vértice e geometrias maiores que 4096 vértices.

O modo Balanceado é o teste inicial recomendado. Os outros podem
ocasionar *popping* de cenário, iluminação excessivamente escura ou
efeitos desaparecendo. Se a imagem estiver errada, volte ao estável.

## Mudanças técnicas comprováveis no código

1. Em `gx_gl.cpp`, a geração de `ShaderKey` mantém iluminação de
   material/ambiente mas reduz a quantidade de luzes dinâmicas calculadas
   por vértice. A GPU executa shaders potencialmente mais simples. Os
   shaders compilados são diferentes e usados somente na versão Low Spec.
2. Em BP `0x5A`, ignora a etapa de *autofocus* que gera e executa
   múltiplas passagens de desfoque de profundidade; o resto dos comandos
   GX continua em ordem.
3. Na variante Ultra, o shader fragment deixa de aplicar neblina GX;
   isso altera a estética e não necessariamente reduz muitos draws.
4. Na variante PS2 Leve ou Ultra, detecta objetos completamente
   distantes antes de `ProgramFor`, `ApplyState`, uploads de uniforms
   e `glDrawElements`; se elegível, pula a chamada inteira.
   **Não reduz a geometria dos personagens**, nem faz LOD automático.
5. Mantém o cache persistente existente de programas GLES3 da Mali,
   com `GL_PROGRAM_BINARY_RETRIEVABLE_HINT`/`glGetProgramBinary`,
   `glProgramBinary` e fallback de compilação se necessário.

**Não tocamos** no `glFinish()` antes do `SDL_GL_SwapWindow()`, porque
foi ele que evitou a queda no quadro 16. VSync permanece solicitado
em 1, BaseVertex desligado, sem modificações no processo `re4_coreproc`.

## Instalação

Baixe o artifact `CubeEvil4-R36S-PS2-LowSpec-ARM64` do GitHub Actions
da branch `chatgpt/r36s-ps2-lowspec-renderer`. Extraia o ZIP em
`/roms/ports`. Ele **adiciona**:

- `cubeevil4_live_gx/r36s_game_ps2`
- `RE4-R36S-PS2-LowSpec.sh`
- `RE4-R36S-PS2-Balanceado.sh`
- `RE4-R36S-PS2-Leve.sh`
- `RE4-R36S-PS2-Ultra.sh`
- `LEIA-ME-PS2-LOW-SPEC.md`

Preserve **todos os seus arquivos existentes**, especialmente
`r36s_game_boot`, `RE4-R36S-Estavel.sh`, `re4_coreproc`,
`modules/`, `data/` e `data/memcard/card-a.bin`. Recomenda-se
backup do cartão de memória virtual antes dos testes.

## Logs e como comparar

Primeiro execute `RE4-R36S-PS2-Balanceado.sh`, entre na primeira
área e teste por 180 s (ou saia com SELECT+START por 2 s, se SELECT
mapear para SDL BACK). Depois opcionalmente teste o perfil PS2 Leve,
e por último Ultra se for necessário.

Cada perfil tem seu próprio relatório:

- `/roms/ports/cubeevil4_live_gx/diagnostico_ps2_balanceado_darkos.txt`
- `/roms/ports/cubeevil4_live_gx/diagnostico_ps2_ps2_darkos.txt`
- `/roms/ports/cubeevil4_live_gx/diagnostico_ps2_ultra_darkos.txt`

Procure `LOW_SPEC_ENABLED`, `R36S_LOWSPEC`,
`LOW_SPEC_WINDOW`, `LOW_SPEC_CULLED_DRAWS`,
`DOF_PASSES_SKIPPED`, `FPS_PRESENTED`, `SHADERS`, 
`SHADER_CACHE_FINAL_HITS`, `GL_ERRORS` e `EXIT_CODE`.
Queremos um aumento de FPS medido **na mesma área e com o mesmo
trajeto**, sem defeitos visuais que prejudiquem a jogabilidade.

A pasta do cache é compartilhada entre os experimentos em
`cubeevil4_live_gx/cache/shader_binaries/`. Novos shaders simplificados
têm chaves derivadas do texto de seus fontes; por isso podem criar novos
arquivos, sem apagar os anteriores. No log de primeira execução
`SHADER_CACHE_FINAL_HITS` não precisa ser zero: o núcleo pode emitir
chunks `RESET` e os programas reaproveitam o cache gravado segundos
antes nessa mesma execução. No primeiro teste anterior foram gerados
99 arquivos (aproximadamente 1,8 MiB), com 29 hits e sem erros.

## Limitações importantes

- **Sem áudio SDL2 conectado** no frontend atual.
- A GPU é uma Mali-G31; a RAM livre não resolve gargalos de Draw() ou
  compilação de shaders que já estão em cache.
- Cache de shaders persistente reduz carregamento/travadinhas; não deve
  elevar FPS estável em cenas já renderizadas.
- Nem todos os modelos usam uma matriz de posição única. O culling
  conservador ignora os outros e pode reduzir poucos draws. Se o
  contador `CULLED_DRAWS=0`, isso não representa falha do patch: apenas
  ausência de geometrias que cumpram o filtro.
- Desativar DOF e iluminação muda a estética; corte de geometria pode
  fazer elementos aparecerem repentinamente. **Esta não é uma reprodução
  literal da versão PS2 da Capcom**.
- O processo do jogo e o renderer compartilham RAM da máquina;
  sem swap, evite lotar memória com caches sem limites.
