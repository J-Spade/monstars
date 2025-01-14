# Generated by Django 3.2.12 on 2024-07-19 16:21

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='BlankoPlayer',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('hostname', models.CharField(max_length=128)),
                ('address', models.CharField(max_length=64)),
                ('kernel', models.CharField(max_length=64)),
                ('active', models.BooleanField(default=False, verbose_name='whether the player is still active')),
                ('birthday', models.DateTimeField(verbose_name='date installed')),
            ],
        ),
        migrations.CreateModel(
            name='BlankoPlay',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('play_time', models.DateTimeField(verbose_name='when the play was sent to the player')),
                ('verb', models.CharField(max_length=16)),
                ('scored', models.BooleanField(verbose_name='whether the play was successful')),
                ('detail', models.CharField(default='', max_length=20000)),
                ('penalty', models.CharField(default='', max_length=255)),
                ('filepath', models.CharField(default='', max_length=300)),
                ('player', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='blanko.blankoplayer')),
            ],
        ),
    ]
