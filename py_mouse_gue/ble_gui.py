import asyncio
import threading
import colorsys
from tkinter import Tk, Button, Label, Scale, HORIZONTAL, Canvas, Frame
from PIL import Image, ImageTk
from bleak import BleakClient, BleakScanner
import math

TARGET_NAME = "Ball Mouse"

COLOR_SERVICE_UUID          = "12345678-1234-5678-1234-56789abcdef0"
COLOR_CHAR_R_UUID           = "12345678-1234-5678-1234-56789abcdef1"
COLOR_CHAR_G_UUID           = "12345678-1234-5678-1234-56789abcdef2"
COLOR_CHAR_B_UUID           = "12345678-1234-5678-1234-56789abcdef3"
COLOR_CHAR_BRIGHTNESS_UUID  = "12345678-1234-5678-1234-56789abcdef4"

# Часто используемые цвета (R, G, B)
preset_colors = [
    (255, 0, 0),     # Красный
    (0, 255, 0),     # Зеленый
    (0, 0, 255),     # Синий
    (255, 255, 0),   # Желтый
    (255, 0, 255),   # Пурпурный
    (0, 255, 255),   # Голубой (циан)
    (255, 255, 255)  # Белый
]

class BLEController:
    def __init__(self):
        self.client = None

    async def connect(self):
        devices = await BleakScanner.discover()
        device = next((d for d in devices if d.name and TARGET_NAME in d.name), None)
        if not device:
            print("BLE-устройство не найдено")
            return False
        self.client = BleakClient(device.address)
        try:
            await self.client.connect()
            print("Подключено к", device.name)
            return True
        except Exception as e:
            print("Ошибка подключения:", e)
            return False

    async def send_color_components(self, r, g, b, brightness=255):
        if not self.client or not self.client.is_connected:
            print("Не подключено к устройству")
            return False
        try:
            await self.client.write_gatt_char(COLOR_CHAR_R_UUID, bytes([r]))
            await self.client.write_gatt_char(COLOR_CHAR_G_UUID, bytes([g]))
            await self.client.write_gatt_char(COLOR_CHAR_B_UUID, bytes([b]))
            await self.client.write_gatt_char(COLOR_CHAR_BRIGHTNESS_UUID, bytes([brightness]))
            print(f"Отправлен цвет: R={r} G={g} B={b} яркость={brightness}")
            return True
        except Exception as e:
            print("Ошибка при отправке:", e)
            return False
            
    async def read_color_and_brightness(self):
        if not self.client or not self.client.is_connected:
            print("Не подключено к устройству")
            return None
        try:
            r_bytes = await self.client.read_gatt_char(COLOR_CHAR_R_UUID)
            g_bytes = await self.client.read_gatt_char(COLOR_CHAR_G_UUID)
            b_bytes = await self.client.read_gatt_char(COLOR_CHAR_B_UUID)
            brightness_bytes = await self.client.read_gatt_char(COLOR_CHAR_BRIGHTNESS_UUID)
            r = int.from_bytes(r_bytes, byteorder="little")
            g = int.from_bytes(g_bytes, byteorder="little")
            b = int.from_bytes(b_bytes, byteorder="little")
            brightness = int.from_bytes(brightness_bytes, byteorder="little")
            return (r, g, b, brightness)
        except Exception as e:
            print("Ошибка чтения цвета и яркости:", e)
            return None
            
          

def create_color_wheel(size):
    radius = size // 2
    image = Image.new("RGB", (size, size), (255, 255, 255))
    pixels = image.load()

    for x in range(size):
        for y in range(size):
            dx = x - radius
            dy = y - radius
            dist = math.sqrt(dx*dx + dy*dy)
            if dist <= radius:
                hue = (math.atan2(dy, dx) / (2 * math.pi)) + 0.5
                saturation = dist / radius
                r, g, b = colorsys.hsv_to_rgb(hue, saturation, 1)
                pixels[x, y] = (int(r*255), int(g*255), int(b*255))
            else:
                pixels[x, y] = (255, 255, 255)
    return image

def run_gui():
    brightness_send_job = None
    brightness_delay_ms = 100  # например, 100 мс задержки

    ble = BLEController()

    root = Tk()
    root.title("BLE RGB Контроллер")

    label = Label(root, text="Ожидание подключения...")
    label.pack(pady=10)

    brightness_scale = Scale(root, from_=0, to=100, orient=HORIZONTAL, label="Яркость", length=300)
    brightness_scale.set(100)
    brightness_scale.pack(pady=10)

    canvas_size = 300
    radius = canvas_size // 2

    canvas = Canvas(root, width=canvas_size, height=canvas_size)
    canvas.pack()

    pil_image = create_color_wheel(canvas_size)
    tk_image = ImageTk.PhotoImage(pil_image)
    canvas.create_image(0, 0, anchor="nw", image=tk_image)

    selected_color = [0, 0, 0]    
    
    # Создаем фрейм для кнопок с цветами
    preset_frame = Frame(root)
    preset_frame.pack(pady=10)

    # Квадрат для отображения итогового цвета
    color_display = Frame(root, width=100, height=100, bg="#000000", relief="sunken", borderwidth=2)
    color_display.pack(pady=10)
  
    def on_read():
        label.config(text="Чтение данных...")

        def read_and_update():
            color_bright = asyncio.run(ble.read_color_and_brightness())
            if color_bright is None:
                root.after(0, lambda: label.config(text="Ошибка чтения данных"))
                return

            r, g, b, brightness = color_bright
            selected_color[0], selected_color[1], selected_color[2] = r, g, b

            def update_gui():
                brightness_scale.set(brightness)
                update_color_display(r, g, b, brightness)
                label.config(text=f"Считано: R={r} G={g} B={b}, яркость={brightness}")

            root.after(0, update_gui)

        threading.Thread(target=read_and_update).start()
            
    def update_color_display(r, g, b, brightness):
        # Учитываем яркость — просто умножаем компоненты на коэффициент
        factor = brightness / 100
        r_adj = int(r * factor)
        g_adj = int(g * factor)
        b_adj = int(b * factor)
        hex_color = f"#{r_adj:02x}{g_adj:02x}{b_adj:02x}"
        color_display.config(bg=hex_color)

    def send_color_and_update(r, g, b):
        brightness = brightness_scale.get()

        async def task():
            return await ble.send_color_components(r, g, b, brightness)

        def thread_func():
            result = asyncio.run(task())
            def update_label():
                if result:
                    label.config(text="Цвет установлен")
                else:
                    label.config(text="Цвет не установлен")
            root.after(0, update_label)

        threading.Thread(target=thread_func).start()
        update_color_display(r, g, b, brightness)

    def on_canvas_click(event):
        dx = event.x - radius
        dy = event.y - radius
        dist = math.sqrt(dx*dx + dy*dy)
        if dist > radius:
            return
        hue = (math.atan2(dy, dx) / (2 * math.pi)) + 0.5
        saturation = dist / radius
        r, g, b = colorsys.hsv_to_rgb(hue, saturation, 1)
        r, g, b = int(r*255), int(g*255), int(b*255)
        selected_color[0], selected_color[1], selected_color[2] = r, g, b
        send_color_and_update(r, g, b)

    canvas.bind("<Button-1>", on_canvas_click)
    
    
    def on_preset_click(r, g, b):
        selected_color[0], selected_color[1], selected_color[2] = r, g, b
        send_color_and_update(r, g, b)

    for color in preset_colors:
        r, g, b = color
        hex_color = f"#{r:02x}{g:02x}{b:02x}"
        btn = Button(preset_frame, bg=hex_color, width=3, height=1,
                     command=lambda r=r, g=g, b=b: on_preset_click(r, g, b))
        btn.pack(side="left", padx=3)

    
    
    
    def delayed_send_brightness():
        nonlocal brightness_send_job
        brightness_send_job = None
        r, g, b = selected_color
        send_color_and_update(r, g, b)
        
    def on_brightness_change(val):
        nonlocal brightness_send_job
        if brightness_send_job is not None:
            root.after_cancel(brightness_send_job)
        brightness_send_job = root.after(brightness_delay_ms, delayed_send_brightness)

    brightness_scale.config(command=on_brightness_change)


    def on_connect():
        label.config(text="Подключение...")

        def connect_and_sync():
            result = asyncio.run(ble.connect())
            if not result:
                root.after(0, lambda: label.config(text="Ошибка подключения"))
                return

            # Читаем цвет и яркость из устройства
            color_bright = asyncio.run(ble.read_color_and_brightness())
            if color_bright is None:
                root.after(0, lambda: label.config(text="Не удалось прочитать цвет"))
                return

            r, g, b, brightness = color_bright
            selected_color[0], selected_color[1], selected_color[2] = r, g, b

            # Обновляем GUI в основном потоке
            def update_gui():
                brightness_scale.set(brightness)
                update_color_display(r, g, b, brightness)
                label.config(text=f"Подключено: R={r} G={g} B={b}, яркость={brightness}")

            root.after(0, update_gui)

            # Отправляем цвет-яркость на устройство для синхронизации
            send_color_and_update(r, g, b)

        threading.Thread(target=connect_and_sync).start()





    connect_btn = Button(root, text="Подключиться к мыши", command=on_connect)
    connect_btn.pack(pady=10)
    
    read_btn = Button(root, text="Считать", command=on_read)
    read_btn.pack(pady=10)

    root.mainloop()

if __name__ == "__main__":
    run_gui()
