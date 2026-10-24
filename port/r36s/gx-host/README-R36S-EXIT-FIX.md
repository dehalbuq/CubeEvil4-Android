# CubeEvil4 R36S — correção do comando SAIR (DarkOSRE)

## Por que o menu não abriu no controle

A demonstração anterior consultava principalmente os botões virtuais
`SDL_CONTROLLER_BUTTON_BACK` (SELECT), `GUIDE` e
`SDL_CONTROLLER_BUTTON_START + L1 + R1`. Dependendo do mapeamento do
controle embutido do R36S no DarkOSRE, SELECT e os ombros podem não
gerar esses eventos padronizados, mesmo com a jogabilidade funcionando.

A versão corrigida escuta **também eventos físicos
`SDL_JOYBUTTONDOWN`** e lê diretamente
`SDL_JoystickGetButton` no joystick selecionado, além de continuar
lendo `SDL_GameControllerGetButton`. Aceita layouts usuais com
SELECT/START em botões 8/9 ou 6/7, ou valores explicitamente
configurados. O jogo segue recebendo os controles normais.

## Saída que não depende de SELECT ou do menu

| Controle físico R36S | Efeito |
|---|---|
| **Segure START sozinho por 2,5 segundos** | Abre o menu para sair |
| **Continue segurando START por 6 segundos** | **SAI DIRETAMENTE**, sem precisar de SELECT, A ou B |
| SELECT (clique) | Abre/fecha o menu, se reconhecido pelo SDL |
| **START + SELECT por 1,5 segundo** | Sai diretamente para Ports |
| START + L1 + R1 por um segundo | Abre o menu (quando os ombros estiverem mapeados) |
| A com o menu aberto | Confirma saída |
| B com o menu aberto | Cancela e volta ao jogo |
| Teclado ESC/F1 | Abre/fecha o menu |
| Teclado Enter/Z ou Backspace/X | Confirmar ou cancelar o menu |

**Primeiro teste recomendado:** com a partida já iniciada, segure
START **sem soltar por seis segundos**. O menu pode aparecer
antes; mesmo que não apareça ou A/B não sejam reconhecidos, o próprio
programa solicita desligamento do núcleo, devolve o SDL/KMSDRM e volta
ao menu Ports. Não precisa de SELECT ou da combinação de ombros.
Se o botão START não chegar ao SDL, precisamos conferir a numeração
física do controle no aparelho (diagnóstico opcional abaixo).

O atalho direto é uma saída **normal** da aplicação, não uma
interrupção forçada com `kill -9`; a rotina já existente envia pad
neutro, fecha a conexão IPC com o núcleo, libera áudio, GLES, janela e
SDL. A única forma de sair caso o firmware inteiro esteja travado
será o mecanismo de recuperação do sistema.

## Como instalar

Artifact no GitHub Actions:
**`CubeEvil4-R36S-Demo-Saida-Corrigida-ARM64`**.

Extraia em `/roms/ports` com opção de **mesclar pastas**.
Isso substitui **somente o script do menu**
`RE4-R36S-Demonstracao-Tela-Cheia.sh` e acrescenta
`cubeevil4_live_gx/r36s_game_showcase_exitfix`.
A demonstração anterior
`r36s_game_showcase_full`, as versões de teste/estável
`r36s_game_boot` e `r36s_game_a35_scalable`, `re4_coreproc`,
bibliotecas, módulos, arquivos de disco e saves continuam intactos.

O launcher continua com:

- imagem esticada em **tela cheia 640×480** sem barras pretas
  (resolução interna EFB **480×396**);
- supressão do `[PRINT_PAGE]` original do jogo por cópia temporária
  da configuração (não altera os arquivos da instalação);
- áudio SDL estéreo 32 kHz, shader cache persistente;
- `-O3 -mcpu=cortex-a35 -mtune=cortex-a35 -flto=auto`, 2 luzes
  dinâmicas, DOF/fog off, sem culling;
- **modo GPU 5 com glFinish antes do swap**, mantendo o método de
  sincronização que evitou travamentos na Mali-G31;
- nenhum overlay de FPS, texto de debug do jogo, relatórios ou
  rastreamento pesado durante o uso normal.

## Diagnóstico dos botões se START também falhar

Na própria instalação (via SSH ou terminal), faça:

```sh
RE4_SHOWCASE_INPUT_DIAG=1 sh /roms/ports/RE4-R36S-Demonstracao-Tela-Cheia.sh
```

Pressione uma vez SELECT, START, A e B, e saia com o botão que funcionar
ou usando o menu Ports do sistema. Isso cria SOMENTE o arquivo:

```text
/roms/ports/cubeevil4_live_gx/logs/diagnostico_controles_demo_darkos.txt
```

O arquivo contém `R36S_RAW_JOYBUTTON_DOWN DEVICE=... BUTTON=...` e
a detecção do controle; **nada é mostrado sobre o jogo**. Envie o TXT
para que possamos mapear exatamente o R36S com o firmware DarkOSRE.

Você também pode selecionar números do joystick manualmente, antes
de abrir o script:

```sh
RE4_SHOWCASE_RAW_SELECT=8 RE4_SHOWCASE_RAW_START=9 \
  sh /roms/ports/RE4-R36S-Demonstracao-Tela-Cheia.sh
```

Em alguns dispositivos, os índices são 6 e 7. Sem configuração,
o código já reconhece ambas as duplas quando SELECT/START
não têm binding SDL GameController.

## Status dos testes

O workflow usa testes C++ **sem dados de jogo**: tap de SELECT
abre/fecha uma vez, A/B confirmam/cancelam apenas com menu
visível, START sozinho 2,5s abre menu / 6s sai, START+SELECT 1,5s
sai direto, START+L1+R1 abre menu, soltar START zera temporizadores,
e joystick virtual do SDL valida os raw buttons 8/9.

A compilação AArch64 valida link/arquitetura. **Ainda precisamos
confirmar no R36S real qual evento de START o DarkOSRE fornece**,
bem como os sons e retorno ao menu do sistema.

O pacote, como nas versões anteriores, contém somente o frontend
experimental e script; não inclui arquivos comerciais nem o núcleo
recompilado, que precisam ser instalados legalmente e de maneira
compatível pelo próprio usuário.
