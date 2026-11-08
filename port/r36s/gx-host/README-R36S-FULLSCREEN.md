# CubeEvil4 R36S — demonstração Fullscreen 4:3 sem textos de depuração

**Status:** build experimental. Ainda exige validação visual no console físico.
A versão anterior continua intacta.

## Alterações solicitadas

- **Preenchimento total da tela 640×480 do R36S:** o shader de
  apresentação usa **100% da largura da imagem original**, remove
  aproximadamente as faixas pretas superior/inferior (12,5% de cada)
  e estica o conteúdo verticalmente para preencher a tela.
  **Não preserva o aspecto 16:9** e pode deformar personagens;
  isso foi solicitado expressamente para usar a tela toda.
  NÃO aumenta o campo de visão nem mexe na geometria, câmeras e HUD.
- **Nenhuma informação de debug sobreposta ao gameplay:** desativa
  os textos técnicos originais do RE4 GameCube debug build via
  `[PRINT_PAGE] 0` em uma **cópia TEMPORÁRIA do config.txt** e
  retira as opções de aspecto da interface da demonstração.
  O menu restante só apresenta **SAIR DO JOGO? / A: SAIR / B: VOLTAR**.
- **Proteção absoluta dos arquivos originais:** o frontend cria
  `cubeevil4_live_gx/cache/showcase-clean-XXXXXX/` com links simbólicos
  para os diretórios `disc0/sys`, `disc0/files/*` (exceto
  `debug/config.txt`), `disc1` e `memcard`.
  O único arquivo copiado é `debug/config.txt` — modifica apenas
  os dígitos após `[PRINT_PAGE]` para `0`, **sem alterar seu
  tamanho em bytes**, pois o FST do GameCube guarda o comprimento.
  Todos os outros itens, inclusive saves, permanecem no local original.
  Ao encerrar, exclui a pasta temporária sem atravessar os symlinks.
  Se não localizar o arquivo original ou a tag `[PRINT_PAGE]`,
  usa os dados originais e as mensagens de debug podem permanecer.
  Isso não significa falha da imagem principal.
- **Sem profiler GPU nem relatórios na tela ou na sessão:** o binário
  não instala rastreamento de travamentos, não ativa probes de GPU,
  desativa hash IPC adicional, não executa o coletor de FPS/RSS
  a cada cinco segundos e o script silencia stdout/stderr por padrão.
  Mensagens do próprio jogo não podem ser confundidas com o HUD normal:
  vida, munição, mapas, legendas e inventário permanecem intactos.
- **Som estéreo PCM 32kHz pelo SDL2**, via thread independente;
  canal de áudio da Mali/ALSA ainda exige teste físico.
- **Compilação Cortex-A35 O3 LTO**, resolução 480×396, cache shader Mali,
  duas luzes, DOF/fog desativados, sem remoção de geometria.
- **Mantém VSync1 + glFinish antes do swap**, pois essa
  sincronização evitou crashes anteriores.

## Instalar / executar

Artifact GitHub Actions:
`CubeEvil4-R36S-Demo-Fullscreen-Sem-Debug-ARM64`.

Extraia em `/roms/ports` sem excluir os arquivos que já existem.
O ZIP contém somente os três itens abaixo:

```text
/roms/ports/
  RE4-R36S-Demonstracao-Tela-Cheia.sh
  LEIA-ME-R36S-FULLSCREEN.md
  cubeevil4_live_gx/
    r36s_game_showcase_full
```

**No DarkOSRE, execute `RE4-R36S-Demonstracao-Tela-Cheia.sh`**.
Seu launcher `RE4-R36S-Demonstracao-43.sh` anterior e
`RE4-R36S-Estavel.sh` permanecem intactos.

### Controles

- **SELECT/BACK ou GUIDE/HOME:** abre menu "SAIR DO JOGO?"
- **START + L1 + R1 por um segundo:** atalho alternativo se SELECT
  não funciona no DarkOSRE
- **A:** sair do jogo e voltar ao menu Ports
- **B:** fechar menu e continuar a partida
- teclado: ESC/F1 abre, Enter/Z confirma, Backspace/X cancela

Não há alternância de resolução ou modo de imagem durante a sessão.
O jogo permanece em fullscreen esticado o tempo inteiro.

A sessão dura até duas horas ou até o usuário escolher Sair. Para
recolher mensagens de erros somente durante desenvolvimento use:

```sh
RE4_SHOWCASE_VERBOSE=1 sh /roms/ports/RE4-R36S-Demonstracao-Tela-Cheia.sh
```

## Testes automatizados e ressalvas

O CI usa modelo de diretório fictício e configuração de debug
**sintética**, comprova `[PRINT_PAGE] 17 → 00`, preservação de
comprimento, symlinks para FST e memcard, limpeza da pasta e os
arquivos originais inalterados.

Uma compilação arm64 e teste de arquivo **não comprovam** que o
jogo ficará perfeito no monitor físico, nem que o debug não volta
por outro menu original. Também não comprovam som audível no R36S;
exigem teste no dispositivo real.

Se persistir alguma informação de debug na tela, envie uma foto para
identificarmos a segunda fonte de renderização. Não aplique máscaras
retangulares que apaguem munição, itens ou legendas.

## Distribuição

O artifact contém só o frontend, o launcher e a documentação.
Não inclui o núcleo `re4_coreproc`, módulos, bibliotecas recompiladas,
memcard ou os arquivos comerciais de Resident Evil 4. Uma instalação
compatível e obtida legitimamente continua necessária.

O README/LICENSE do projeto distingue o código reconstruído do
jogo (direitos dos titulares) de scripts e documentação CC0.
Antes de redistribuir outros binários ou assets, confirme os direitos.
A alternativa mais simples para apresentar o progresso é publicar
vídeo da execução no R36S e os resultados dos testes.
