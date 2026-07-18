extends Area2D



const SCREEN_WIDTH = 320
var vector := Vector2(-1,0);
var phase;

func _process(delta):
	position += vector  * delta
	if ((position.x < -8 && phase != Settings.Phase.HIDE) or position.x > SCREEN_WIDTH + 8) :   
		queue_free()


func _on_shot_area_entered(area):
	if area.is_in_group("asteroid") or area.is_in_group("low_nemy"):
		queue_free()
		
