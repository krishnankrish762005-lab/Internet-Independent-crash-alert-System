import sqlite3
import re
from flask import Flask, render_template, request, redirect, url_for, flash

app = Flask(__name__)
app.secret_key = 'sos_vehicle_key'

def get_db():
    conn = sqlite3.connect('vehicle_sos.db')
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    db = get_db()
    db.execute('CREATE TABLE IF NOT EXISTS vehicles (id INTEGER PRIMARY KEY AUTOINCREMENT, vehicle TEXT NOT NULL, emergency1 TEXT NOT NULL, emergency2 TEXT NOT NULL)')
    db.commit()
    db.close()

def validate_vehicle_id(v):
    v = v.strip().upper()
    pattern = r'^[A-Z]{2}[\s\-]\d{2}[\s\-][A-Z]{1,2}[\s\-]\d{4}$'
    return v if re.match(pattern, v) else None

def format_contact(number):
    number = number.strip().replace(" ", "")
    if number.startswith('+91') and len(number) == 13:
        return number
    if number.startswith('91') and len(number) == 12:
        return '+' + number
    if number.isdigit() and len(number) == 10:
        return '+91' + number
    return None

@app.route('/')
def index():
    db = get_db()
    rows = db.execute('SELECT * FROM vehicles ORDER BY id DESC').fetchall()
    db.close()
    return render_template('index.html', rows=rows)

@app.route('/save', methods=['POST'])
def save():
    v  = request.form.get('vehicle', '').strip()
    e1 = request.form.get('emergency1', '').strip()
    e2 = request.form.get('emergency2', '').strip()

    vehicle = validate_vehicle_id(v)
    if not vehicle:
        flash('Invalid Vehicle ID. Use AA-00-AA-0000', 'error')
        return redirect(url_for('index'))

    contact1 = format_contact(e1)
    contact2 = format_contact(e2)

    if not contact1:
        flash('Invalid Contact 1. Enter 10-digit number.', 'error')
        return redirect(url_for('index'))

    if not contact2:
        flash('Invalid Contact 2. Enter 10-digit number.', 'error')
        return redirect(url_for('index'))

    db = get_db()
    db.execute('INSERT INTO vehicles (vehicle, emergency1, emergency2) VALUES (?, ?, ?)', (vehicle, contact1, contact2))
    db.commit()
    db.close()
    flash('Vehicle saved successfully!', 'success')
    return redirect(url_for('index'))

@app.route('/delete/<int:id>')
def delete(id):
    db = get_db()
    db.execute('DELETE FROM vehicles WHERE id = ?', (id,))
    db.commit()
    db.close()
    flash('Record deleted.', 'info')
    return redirect(url_for('index'))

@app.route('/get_latest_data')
def get_latest_data():
    db = get_db()
    row = db.execute('SELECT vehicle, emergency1, emergency2 FROM vehicles ORDER BY id DESC LIMIT 1').fetchone()
    db.close()
    if row and row[0] and row[1] and row[2]:
        return f"CONFIG:{row[0]},{row[1]},{row[2]}"
    return "NO_DATA"

if __name__ == '__main__':
    init_db()
    app.run(debug=True, port=5000)