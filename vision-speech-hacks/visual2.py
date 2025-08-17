import cv2
import numpy as np
import mediapipe as mp
import time

# --- Mediapipe Hands setup ---
mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils
hands = mp_hands.Hands(min_detection_confidence=0.5, min_tracking_confidence=0.5)

# --- Commands for grid ---
commands = ["START", "QUIT", "BICEPS", "COBRA", "STOP", "EXIT"]

# Grid: 2 rows x 3 cols
rows, cols = 2, 3
cell_w, cell_h = 250, 150
menu_w, menu_h = cols * cell_w, rows * cell_h
banner_h = 200
window_h = menu_h + banner_h
window_w = menu_w

cv2.namedWindow("Finger Menu", cv2.WINDOW_NORMAL)

# Selection tracking
last_cell = None
cell_start_time = 0
selected_command = ""

cap = cv2.VideoCapture(0)

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    frame = cv2.flip(frame, 1)  # mirror
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    results = hands.process(rgb)

    # White canvas (menu + banner)
    canvas = 255 * np.ones((window_h, window_w, 3), dtype="uint8")

    # --- Draw grid + labels ---
    for r in range(rows):
        for c in range(cols):
            x1, y1 = c * cell_w, r * cell_h
            x2, y2 = x1 + cell_w, y1 + cell_h
            idx = r * cols + c
            label = commands[idx]

            color = (200, 200, 200)
            if label == selected_command:
                color = (0, 255, 0)

            cv2.rectangle(canvas, (x1, y1), (x2, y2), color, 3)
            text_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 1.2, 3)[0]
            text_x = x1 + (cell_w - text_size[0]) // 2
            text_y = y1 + (cell_h + text_size[1]) // 2
            cv2.putText(canvas, label, (text_x, text_y),
                        cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 0), 3, cv2.LINE_AA)

    # --- Process hand landmarks ---
    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:
            mp_drawing.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

            # Index fingertip = landmark 8
            x = int(hand_landmarks.landmark[8].x * menu_w)
            y = int(hand_landmarks.landmark[8].y * menu_h)

            cv2.circle(canvas, (x, y), 10, (0, 0, 255), -1)

            # Detect which cell finger is in
            cell_col = x // cell_w
            cell_row = y // cell_h
            if 0 <= cell_row < rows and 0 <= cell_col < cols:
                current_cell = (cell_row, cell_col)
                idx = cell_row * cols + cell_col
                command_here = commands[idx]

                # Selection logic: hold for 1s
                if current_cell == last_cell:
                    if time.time() - cell_start_time > 1.0:
                        selected_command = command_here
                else:
                    last_cell = current_cell
                    cell_start_time = time.time()

    # --- Draw big banner at bottom ---
    if selected_command:
        banner_text = f"COMMAND: {selected_command}"
    else:
        banner_text = "Move finger to select a command"

    text_scale = 2.5
    thickness = 6
    text_size = cv2.getTextSize(banner_text, cv2.FONT_HERSHEY_SIMPLEX, text_scale, thickness)[0]
    text_x = (window_w - text_size[0]) // 2
    text_y = menu_h + (banner_h + text_size[1]) // 2
    cv2.putText(canvas, banner_text, (text_x, text_y),
                cv2.FONT_HERSHEY_SIMPLEX, text_scale, (0, 0, 0), thickness, cv2.LINE_AA)

    # --- Show menu ---
    cv2.imshow("Finger Menu", canvas)

    if cv2.waitKey(10) & 0xFF == 27:  # ESC
        break

cap.release()
cv2.destroyAllWindows()
