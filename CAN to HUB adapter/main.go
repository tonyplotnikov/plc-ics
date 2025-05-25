package main

import (
	"encoding/binary"
	"encoding/json"
	"fmt"
	"log"
	"math"
	"os"
	"os/signal"
	"strconv"
	"syscall"

	"github.com/brutella/can"
	"github.com/gorilla/websocket"
)

// HubConfig
type HubConfig struct {
	URL string `json:"url"`
}

// CANMessageID
type CANMessageID struct {
	Hub    string `json:"hub"`
	Entity string `json:"entity"`
	Param  string `json:"param"`
	Type   string `json:"type"` // +поле типа
}

// Config
type Config struct {
	CanIf     string                  `json:"if"`
	CanSpeed  int                     `json:"speed"`
	Hubs      map[string]HubConfig    `json:"hubs"`
	CANMsgIDs map[string]CANMessageID `json:"can_msg_ids"`
}

// Вебсокет клиент для подключения к хабам
type WebSocketClient struct {
	conn *websocket.Conn
	url  string
}

// Подключение к хабу
func (client *WebSocketClient) Connect() error {
	var err error
	client.conn, _, err = websocket.DefaultDialer.Dial(client.url, nil)
	if err != nil {
		return fmt.Errorf("Не удалось подключиться к хабу %s: %w", client.url, err)
	}
	log.Printf("Подключен к хабу: %s", client.url)
	return nil
}

// Отправка данных в хаб через вебксокет
func (client *WebSocketClient) SendMessage(entity string, param string, value string) error {
	message := fmt.Sprintf(`{"entity": "%s", "param": "%s", "value": "%s"}`, entity, param, value)
	return client.conn.WriteMessage(websocket.TextMessage, []byte(message))
}

// Байты в строку
func convertBytesToString(data []byte, dataType string) (string, error) {
	switch dataType {
	case "int":
		if len(data) < 4 {
			return "", fmt.Errorf("Недостаточно байт для преобразования в int")
		}
		val := int(binary.BigEndian.Uint32(data)) // Преобразование в 32-битное целое число
		return strconv.Itoa(val), nil
	case "float":
		if len(data) < 4 {
			return "", fmt.Errorf("Недостаточно байт для преобразования в float")
		}
		bits := binary.BigEndian.Uint32(data) // 32бит
		val := math.Float32frombits(bits)
		return fmt.Sprintf("%f", val), nil
	case "string":
		return string(data), nil
	default:
		return "", fmt.Errorf("Неизвестный тип данных: %s", dataType)
	}
}

// Main block
func main() {
	// Загружаем конифг джсон
	file, err := os.Open("config.json")
	if err != nil {
		log.Fatalf("Не удалось открыть файл конфигурации: %v", err)
	}
	defer file.Close()

	var config Config
	decoder := json.NewDecoder(file)
	if err := decoder.Decode(&config); err != nil {
		log.Fatalf("Не удалось декодировать конфигурацию: %v", err)
	}

	// Подключение к хабу
	hubClients := make(map[string]*WebSocketClient)
	for hubName, hubConfig := range config.Hubs {
		client := &WebSocketClient{url: hubConfig.URL}
		if err := client.Connect(); err != nil {
			log.Fatalf("Ошибка при подключении к хабу %s: %v", hubName, err)
		}
		hubClients[hubName] = client
	}

	// Создание кан интерфейса
	canBus, err := can.NewBusForInterfaceWithName(config.CanIf)
	if err != nil {
		log.Fatalf("Не удалось создать интерфейс CAN %s: %v", config.CanIf, err)
	}

	// Прослушивание can сообщений
	frameChan := make(chan can.Frame)
	canBus.SubscribeFunc(func(frm can.Frame) {
		frameChan <- frm
	})
	go canBus.ConnectAndPublish()

	// Обработка can сообщений
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	for {
		select {
		case frame := <-frameChan:
			msgID := fmt.Sprintf("%d", frame.ID) // id в строку
			if configEntry, exists := config.CANMsgIDs[msgID]; exists {
				// Преобразование байтов в строку в зависимости от типа
				value, err := convertBytesToString(frame.Data[:], configEntry.Type)
				if err != nil {
					log.Printf("Ошибка при преобразовании данных: %v", err)
					continue
				}
				log.Printf("Получено сообщение с ID %s: %s", msgID, value)

				// Отправка сообщения в хаб.
				hubClient, exists := hubClients[configEntry.Hub]
				if !exists {
					log.Printf("Хаб %s не найден", configEntry.Hub)
					continue
				}

				if err := hubClient.SendMessage(configEntry.Entity, configEntry.Param, value); err != nil {
					log.Printf("Ошибка при отправке сообщения: %v", err)
				}
			}
		case sig := <-sigChan:
			log.Printf("Получен сигнал: %s, завершение работы...", sig)
			return
		}
	}
}
