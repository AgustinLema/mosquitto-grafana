FROM python:3.6
COPY src/ /app/
WORKDIR /app
RUN pip install -r requirements.txt
CMD ["python", "consumer.py"]
