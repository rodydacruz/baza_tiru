extends Node2D

signal score(points, enemy_id)
var enemy_id : int = randi() 
var speed = 100
var score_emitted = false
var explosion_scene = preload("res://scenes/explosion.tscn")


func _process(delta):
	position.x = position.x - (delta * speed)
	


func _on_shot_timer_timeout() -> void:
	shoot()
	
	
func shoot():
	
	var Bullet = preload("res://scenes/shot.tscn")
	var bullet_instance  = Bullet.instantiate()
	bullet_instance.global_position = Vector2(position.x - 20,position.y)
	print("Bullet spawned at: ", bullet_instance.global_position)
	bullet_instance.DIRECTION = Vector2.LEFT
	get_parent().add_child(bullet_instance)
	
	


func _on_area_entered(area: Area2D) -> void:
	if area.is_in_group("shot"):
		if not score_emitted:
			score_emitted = true
			emit_signal("score", 10, enemy_id)
			queue_free()
			var stage_node = get_parent()
			var explosion_instance = explosion_scene.instantiate()
			explosion_instance.position = position
			stage_node.add_child(explosion_instance)
