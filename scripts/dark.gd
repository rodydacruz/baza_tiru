extends Area2D



#var explosion_scene = preload("res://scenes/explosion.tscn")

var move_speed = 100.0
var score_emitted = false

signal score(points, asteroid_id)
var asteroid_id : int = randi()

# Called when the node enters the scene tree for the first time.
func _process(delta):
	position -= Vector2(move_speed * delta,0.0)
	if position.x <= - 100:
		queue_free()
		get_parent().events -= 1;
	
func shoot():
	
	var Bullet = preload("res://scenes/shot.tscn")
	var bullet_instance  = Bullet.instantiate()
	bullet_instance.global_position = Vector2(position.x - 20,position.y)
	print("Bullet spawned at: ", bullet_instance.global_position)
	bullet_instance.DIRECTION = Vector2.LEFT
	
	get_parent().add_child(bullet_instance)


	
			


func _on_area_entered(area: Area2D) -> void:
	if area.is_in_group("shot"):
		emit_signal("score", 1, asteroid_id)
		shoot()
