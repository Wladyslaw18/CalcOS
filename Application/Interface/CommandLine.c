#include "CommandLine.h"
#include "Framebuffer.h"
#include "SerialIO.h"
#include "../Input/Parser.h"
#include "../Output/Formatter.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

static void draw_prompt(CommandLineState* cli) {
    char line_buf[VGA_WIDTH + 1];
    fast_memset(line_buf, ' ', VGA_WIDTH);
    line_buf[VGA_WIDTH] = '\0';
    framebuffer_write(line_buf, vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 0, 24);
    
    framebuffer_write("CALC> ", vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), 0, 24);
    framebuffer_write(cli->buffer, vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 6, 24);
    framebuffer_set_cursor(6 + cli->cursor, 24);
}

void cli_init(CommandLineState* cli) {
    cli->length = 0;
    cli->cursor = 0;
    fast_memset(cli->buffer, 0, CMD_BUFFER_SIZE);
    
    framebuffer_clear();
    framebuffer_write("Bare Metal Calculator OS Initialized.\n", vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK), 0, 0);
    framebuffer_write("Type mathematical expressions or 'help'.\n\n", vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK), 0, 1);
    
    draw_prompt(cli);
}

static int mystrcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void execute_command(CommandLineState* cli, CalculatorState* calc) {
    serial_write_str("Executing: ");
    serial_write_str(cli->buffer);
    serial_write_str("\r\n");

    framebuffer_scroll();
    int output_y = 23;

    char echo_buf[VGA_WIDTH + 1];
    format_string(echo_buf, sizeof(echo_buf), "CALC> %s", cli->buffer);
    framebuffer_write(echo_buf, vga_entry_color(VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK), 0, output_y - 1);

    if (mystrcmp(cli->buffer, "help") == 0) {
        framebuffer_write("Commands: clear, mode [float|int|hex], help", vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK), 0, output_y);
        serial_write_str("Commands: clear, mode [float|int|hex], help\r\n");
    } else if (mystrcmp(cli->buffer, "clear") == 0) {
        framebuffer_clear();
        cli->length = 0;
        cli->cursor = 0;
        fast_memset(cli->buffer, 0, CMD_BUFFER_SIZE);
        draw_prompt(cli);
        return;
    } else if (mystrcmp(cli->buffer, "mode float") == 0) {
        if (calc) calc->mode = 0;
        framebuffer_write("Display Mode: Float", vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), 0, output_y);
        serial_write_str("Display Mode: Float\r\n");
    } else if (mystrcmp(cli->buffer, "mode int") == 0) {
        if (calc) calc->mode = 1;
        framebuffer_write("Display Mode: Int", vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), 0, output_y);
        serial_write_str("Display Mode: Int\r\n");
    } else if (mystrcmp(cli->buffer, "mode hex") == 0) {
        if (calc) calc->mode = 2;
        framebuffer_write("Display Mode: Hex", vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK), 0, output_y);
        serial_write_str("Display Mode: Hex\r\n");
    } else {
        bool success = false;
        if (calc) calc->flags = 0;
        double res = parse_expression(cli->buffer, calc, &success);
        
        char result_buf[VGA_WIDTH + 1];
        if (success) {
            if (calc) {
                for (int i = 3; i > 0; i--) {
                    calc->operands[i] = calc->operands[i - 1];
                }
                calc->operands[0] = res;
                if (calc->op_count < 4) calc->op_count++;
            }

            uint8_t mode = calc ? calc->mode : (uint8_t)0;
            if (mode == 1) {
                format_string(result_buf, sizeof(result_buf), "Result: %d", (int64_t)res);
            } else if (mode == 2) {
                format_string(result_buf, sizeof(result_buf), "Result: 0x%X", (uint64_t)res);
            } else {
                format_string(result_buf, sizeof(result_buf), "Result: %f", res);
            }
            framebuffer_write(result_buf, vga_entry_color(VGA_COLOR_LIGHT_YELLOW, VGA_COLOR_BLACK), 0, output_y);
            serial_write_str(result_buf);
            serial_write_str("\r\n");
        } else {
            if (calc && (calc->flags & 2)) {
                format_string(result_buf, sizeof(result_buf), "Error: Division by Zero");
            } else {
                format_string(result_buf, sizeof(result_buf), "Error: Invalid Expression");
            }
            framebuffer_write(result_buf, vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK), 0, output_y);
            serial_write_str(result_buf);
            serial_write_str("\r\n");
        }
    }

    cli->length = 0;
    cli->cursor = 0;
    fast_memset(cli->buffer, 0, CMD_BUFFER_SIZE);
    
    framebuffer_scroll();
    draw_prompt(cli);
}

void cli_process_char(CommandLineState* cli, CalculatorState* calc, char c) {
    if (c == '\r' || c == '\n') {
        if (cli->length > 0) {
            execute_command(cli, calc);
        }
    } else if (c == '\b' || c == 127) {
        if (cli->cursor > 0) {
            for (uint32_t i = cli->cursor - 1; i < cli->length - 1; i++) {
                cli->buffer[i] = cli->buffer[i + 1];
            }
            cli->length--;
            cli->cursor--;
            cli->buffer[cli->length] = '\0';
            draw_prompt(cli);
        }
    } else if (c >= 32 && c <= 126) {
        if (cli->length < CMD_BUFFER_SIZE - 1) {
            for (uint32_t i = cli->length; i > cli->cursor; i--) {
                cli->buffer[i] = cli->buffer[i - 1];
            }
            cli->buffer[cli->cursor] = c;
            cli->length++;
            cli->cursor++;
            cli->buffer[cli->length] = '\0';
            draw_prompt(cli);
        }
    }
}
